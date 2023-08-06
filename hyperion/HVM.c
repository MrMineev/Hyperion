#include "HVM.h"
#include "value.h"
#include "object.h"
#include "debug.h"
#include "compiler.h"
#include "table.h"
#include "memory.h"
#include "DMODE.h"

// <-- MODULES
#include "std/time_module/time.h"
#include "std/math_module/math.h"
#include "std/type_conversion_module/type_conversion.h"
#include "std/file_io_module/file_io.h"
#include "std/console_module/console.h"
#include "std/list_module/list.h"
#include "std/sys_module/sys.h"
#include "std/os_module/os.h"
#include "std/string_module/string.h"
#include "std/random_module/random.h"
// MODULES -->

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

void replace_character(char* str, char find, char replace) {
  if (str == NULL) {
    return; // Handle NULL string case
  }

  size_t length = strlen(str);
  for (size_t i = 0; i < length; i++) {
    if (str[i] == find) {
      str[i] = replace;
    }
  }
}


char* read_file(char* file_path) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
      fprintf(stderr, "Error opening file: %s\n", file_path);
      return NULL;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* content = (char*)malloc(file_size + 1);
    if (content == NULL) {
      fclose(file);
      fprintf(stderr, "Memory allocation failed.\n");
      return NULL;
    }
    size_t bytes_read = fread(content, 1, file_size, file);
    if (bytes_read != file_size) {
      fclose(file);
      free(content);
      fprintf(stderr, "Error reading file: %s\n", file_path);
      return NULL;
    }
    content[file_size] = '\0';
    fclose(file);
    return content;
}

HVM hvm;

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

  hvm.initString = NULL;
  hvm.initString = copy_string("init", 4);

  // define_native("clock", clock_native_function);
}

void free_hvm() {
  free_table(&hvm.globals);
  free_table(&hvm.strings);

  hvm.initString = NULL;

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
      case OBJ_BOUND_METHOD: {
        ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
        hvm.top[-cnt - 1] = bound->receiver;
        return call(bound->method, cnt);
      }
      case OBJ_CLASS: {
        ObjClass* _class = AS_CLASS(callee);
        hvm.top[-cnt - 1] = OBJ_VAL(create_instance(_class));
        Value initializer;
        if (table_get(&_class->methods, hvm.initString, &initializer)) {
          return call(AS_CLOSURE(initializer), cnt);
        } else if (cnt != 0) {
          runtime_error("Expected 0 arguments but got %d.", cnt);
          return false;
        }
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

static bool invoke_from_class(ObjClass* _class, ObjString* name, int cnt) {
  Value method;
  if (!table_get(&_class->methods, name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }
  return call(AS_CLOSURE(method), cnt);
}

static bool invoke(ObjString* name, int cnt) {
  Value receiver = peek_c(cnt);

  if (!IS_INSTANCE(receiver)) {
    runtime_error("Only instances have methods.");
    return false;
  }

  ObjInstance* instance = AS_INSTANCE(receiver);

  Value value;
  if (table_get(&instance->fields, name, &value)) {
    hvm.top[-cnt - 1] = value;
    return call_value(value, cnt);
  }

  return invoke_from_class(instance->_class, name, cnt);
}

static bool bind_method(ObjClass* _class, ObjString* name) {
  Value method;
  if (!table_get(&_class->methods, name, &method)) {
    runtime_error("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod* bound = create_bound_method(peek_c(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
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

static void define_method(ObjString* name) {
  Value method = peek_c(0);
  ObjClass* _class = AS_CLASS(peek_c(1));
  set_table(&_class->methods, name, method);
  pop();
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
    if (DMODE.mode) {
      printf("\t");
      for (Value* pancake = hvm.stack; pancake < hvm.top; pancake++) {
        printf("[ ");
        print_value(*pancake);
        printf(" ]");
      }
      printf("\n");

      debug_instruction(&frame->closure->function->chunk,
          (int)(frame->ip - frame->closure->function->chunk.code));
    }
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_BUILD_LIST: {
        ObjList* list = create_list();
        uint8_t itemCount = READ_BYTE();
        push(OBJ_VAL(list));
        for (int i = itemCount; i > 0; i--) {
          push_back_to_list(list, peek_c(i));
        }
        pop();
        while (itemCount-- > 0) {
          pop();
        }
        push(OBJ_VAL(list));
        break;
      }
      case OP_INDEX_SUBSCR: {
        Value index = pop();
        Value indexable = pop();
        Value result;

        if (!IS_LIST(indexable)) {
          runtime_error("Invalid type to index into.");
          return INTER_RUNTIME_ERROR;
        }

        ObjList* list = AS_LIST(indexable);
        if (!IS_NUMBER(index)) {
          runtime_error("List index is not a number.");
          return INTER_RUNTIME_ERROR;
        } else if (!is_valid_list_index(list, AS_NUMBER(index))) {
          runtime_error("List index out of range.");
          return INTER_RUNTIME_ERROR;
        }
        result = index_from_list(list, AS_NUMBER(index));
        push(result);
        break;
      }
      case OP_STORE_SUBSCR: {
        Value item = pop();
        Value index = pop();
        Value indexable = pop();

         if (!IS_LIST(indexable)) {
          runtime_error("Cannot store value in a non-list.");
          return INTER_RUNTIME_ERROR;
        }

        ObjList* list = AS_LIST(indexable);
        if (!IS_NUMBER(index)) {
            runtime_error("List index is not a number.");
            return INTER_RUNTIME_ERROR;
        } else if (!is_valid_list_index(list, AS_NUMBER(index))) {
          runtime_error("List index out of range.");
          return INTER_RUNTIME_ERROR;
        }
        store_to_list(list, AS_NUMBER(index), item);
        push(item);
        break;
      }
      case OP_INVOKE: {
        ObjString* method = READ_STRING();
        int cnt = READ_BYTE();
        if (!invoke(method, cnt)) {
          return INTER_RUNTIME_ERROR;
        }
        frame = &hvm.frames[hvm.frameCount - 1];
        break;
      }
      case OP_METHOD:
        define_method(READ_STRING());
        break;
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

        if (!bind_method(instance->_class, name)) {
          return INTER_RUNTIME_ERROR;
        }
        break;
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
      case OP_PRINT_TOLINE: {
        print_value(pop());
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
      case OP_IMPORT_MODULE: {
        ObjString *name = READ_STRING();

        char* module_name = name->chars;
        replace_character(module_name, '@', '/');
        char* extension = (char*)".hypl";

        char* module = (char*)malloc(strlen(module_name) + strlen(extension) + 1);
        strcat(module, module_name);
        strcat(module, extension);

        char* source_content = read_file(module);

        return interpret(source_content);
      }
      case OP_IMPORT_STD: {
        ObjString *name = READ_STRING();
        if (strcmp(name->chars, "time") == 0) {
          time_module_init();
        } else if (strcmp(name->chars, "math") == 0) {
          math_module_init();
        } else if (strcmp(name->chars, "type_conv") == 0) {
          type_conversion_module_init();
        } else if (strcmp(name->chars, "file_io") == 0) {
          file_io_module_init();
        } else if (strcmp(name->chars, "console") == 0) {
          console_module_init();
        } else if (strcmp(name->chars, "list") == 0) {
          list_module_init();
        } else if (strcmp(name->chars, "sys") == 0) {
          sys_module_init();
        } else if (strcmp(name->chars, "os") == 0) {
          os_module_init();
        } else if (strcmp(name->chars, "string") == 0) {
          string_module_init();
        } else if (strcmp(name->chars, "random") == 0) {
          random_module_init();
        } else {
          runtime_error("No Standard Module called '%s'", name->chars);
          return INTER_RUNTIME_ERROR;
        }
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
      case OP_POWER: {
        do {
          if (!IS_NUMBER(peek_c(0)) || !IS_NUMBER(peek_c(1))) {
            runtime_error("Operands must be numbers.");
            return INTER_RUNTIME_ERROR;
          }
          int b = (int) AS_NUMBER(pop());
          int a = (int) AS_NUMBER(pop());
          double res = (double) (a ^ b);
          push(NUMBER_VAL(res));
        } while (false);
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

  InterReport res = execute();

  return res;
}


