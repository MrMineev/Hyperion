#include "HVM.h"
#include "value.h"
#include "object.h"
#include "debug.h"
#include "compiler.h"
#include "table.h"
#include "memory.h"

#include <time.h>
#include <stdarg.h>

HVM hvm;

static Value clock_native_function(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static void init_stack() {
  hvm.top = hvm.stack;
  hvm.frameCount = 0;
  hvm.openUpvalues = NULL;
}

static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = hvm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &hvm.frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  init_stack();
}

static void define_native(const char* name, NativeFn function) {
  push(OBJ_VAL(copy_string(name, (int)strlen(name))));
  push(OBJ_VAL(create_native(function)));
  set_table(&hvm.globals, AS_STRING(hvm.stack[0]), hvm.stack[1]);
  pop();
  pop();
}

void init_hvm() {
  init_stack();
  hvm.objects = NULL;

  hvm.bytes_alloc = 0;
  hvm.next_gc_limit = 1024 * 1024;

  hvm.gray_cnt = 0;
  hvm.gray_capacity = 0;
  hvm.gray_stack = NULL;

  init_table(&hvm.globals);
  init_table(&hvm.strings);

  define_native("clock", clock_native_function);
}

void free_hvm() {
  free_table(&hvm.globals);
  free_table(&hvm.strings);

  free_objects();
}

void push(Value value) {
  *hvm.top = value;
  hvm.top++;
}

Value pop() {
  *hvm.top--;
  return *hvm.top;
}

static Value peek_c(int distance) {
  return hvm.top[-1 - distance];
}

static bool call(ObjClosure* closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtime_error("Expected %d arguments but got %d.", closure->function->arity, argCount);
    return false;
  }

  if (hvm.frameCount == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame* frame = &hvm.frames[hvm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = hvm.top - argCount - 1;
  return true;
}

static bool call_value(Value callee, int cnt) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_CLASS: {
        ObjClass* _class = AS_CLASS(callee);
        hvm.top[-cnt - 1] = OBJ_VAL(create_instance(_class));
        return true;
      }
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), cnt);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(cnt, hvm.top - cnt);
        hvm.top -= cnt + 1;
        push(result);
        return true;
      }
      default:
        break;
    }
  }
  runtime_error("Can only call functions and classes.");
  return false;
}

static ObjUpvalue* capture_upvalue(Value* local) {
  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = hvm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue* createdUpvalue = create_upvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    hvm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

static void close_upvalues(Value* last) {
  while (hvm.openUpvalues != NULL &&
         hvm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = hvm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    hvm.openUpvalues = upvalue->next;
  }
}

static bool isFalsey(Value value) {
  return (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString* b = AS_STRING(peek_c(0));
  ObjString* a = AS_STRING(peek_c(1));

  int size = a->size + b->size;
  char* chars = ALLOCATE(char, size + 1);
  memcpy(chars, a->chars, a->size);
  memcpy(chars + a->size, b->chars, b->size);
  chars[size] = '\0';

  ObjString* result = take_string(chars, size);
  pop();
  pop();
  push(OBJ_VAL(result));
}

static InterReport execute() {
  CallFrame* frame = &hvm.frames[hvm.frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() \
    (frame->ip += 2, \
    (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek_c(0)) || !IS_NUMBER(peek_c(1))) { \
      runtime_error("Operands must be numbers."); \
      return INTER_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(valueType(a op b)); \
  } while (false)

  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    /*
    printf("\t");
    for (Value* pancake = hvm.stack; pancake < hvm.top; pancake++) {
      printf("[ ");
      print_value(*pancake);
      printf(" ]");
    }
    printf("\n");
    */

    debug_instruction(&frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));

#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek_c(0))) {
          runtime_error("Only instances have properties.");
          return INTER_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek_c(0));
        ObjString* name = READ_STRING();

        Value value;
        if (table_get(&instance->fields, name, &value)) {
          pop();
          push(value);
          break;
        }

        runtime_error("Undefined property '%s'.", name->chars);
        return INTER_RUNTIME_ERROR;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek_c(1))) {
          runtime_error("Only instances have fields.");
          return INTER_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek_c(1));
        set_table(&instance->fields, READ_STRING(), peek_c(0));
        Value value = pop();
        pop();
        push(value);
        break;
      }
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_CLASS:
        push(OBJ_VAL(create_class(READ_STRING())));
        break;
      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure* closure = create_closure(function);
        push(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] = capture_upvalue(frame->slots + index);
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        close_upvalues(hvm.top - 1);
        pop();
        break;
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek_c(0);
        break;
      }
      case OP_PRINT: {
        print_value(pop());
        printf("\n");
        break;
      }
      case OP_CALL: {
        int cnt = READ_BYTE();
        if (!call_value(peek_c(cnt), cnt)) {
          return INTER_RUNTIME_ERROR;
        }
        frame = &hvm.frames[hvm.frameCount - 1];
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek_c(0))) frame->ip += offset;
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        frame->ip += offset;
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }
      case OP_POP: {
        pop();
        break;               
      }
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek_c(0);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        set_table(&hvm.globals, name, peek_c(0));
        pop();
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value value;
        if (!table_get(&hvm.globals, name, &value)) {
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTER_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (set_table(&hvm.globals, name, peek_c(0))) {
          table_delete(&hvm.globals, name); 
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTER_RUNTIME_ERROR;
        }
        break;
      }
      case OP_NOT: {
        push(BOOL_VAL(isFalsey(pop())));
        break;
      }
      case OP_NEGATE: {
        if (!IS_NUMBER(peek_c(0))) {
          runtime_error("Operand must be a number.");
          return INTER_RUNTIME_ERROR;
        }
        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      }
      case OP_TRUE:
        push(BOOL_VAL(true));
        break;
      case OP_FALSE:
        push(BOOL_VAL(false));
        break;
      case OP_NIL:
        push(NIL_VAL);
        break;
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(are_equal(a, b)));
        break;
      }
      case OP_GREATER:
        BINARY_OP(BOOL_VAL, >);
        break;
      case OP_LESS:
        BINARY_OP(BOOL_VAL, <);
        break;
      case OP_ADD: {
        if (IS_STRING(peek_c(0)) && IS_STRING(peek_c(1))) {
          concatenate();
        } else if (IS_NUMBER(peek_c(0)) && IS_NUMBER(peek_c(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtime_error("Operands must be two numbers or two strings.");
          return INTER_RUNTIME_ERROR;
        }
        break;
      }
      case OP_MINUS: {
        BINARY_OP(NUMBER_VAL, -);
        break;
      }
      case OP_MODULE: {
        do {
          if (!IS_NUMBER(peek_c(0)) || !IS_NUMBER(peek_c(1))) {
            runtime_error("Operands must be numbers.");
            return INTER_RUNTIME_ERROR;
          }
          int b = (int) AS_NUMBER(pop());
          int a = (int) AS_NUMBER(pop());
          double res = (double) (a % b);
          push(NUMBER_VAL(res));
        } while (false);
        break;
      }
      case OP_MULTI: {
        BINARY_OP(NUMBER_VAL, *);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(NUMBER_VAL, /);
        break;
      }
      case OP_RETURN: {
        Value result = pop();
        close_upvalues(frame->slots);
        hvm.frameCount--;
        if (hvm.frameCount == 0) {
          pop();
          return INTER_OK;
        }
        hvm.top = frame->slots;
        push(result);
        frame = &hvm.frames[hvm.frameCount - 1];
        break;
      }
    }
  }

#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
#undef READ_BYTE
#undef READ_SHORT

}

InterReport interpret(const char *source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTER_COMPILE_ERROR;

  push(OBJ_VAL(function));

  ObjClosure* closure = create_closure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return execute();
}


