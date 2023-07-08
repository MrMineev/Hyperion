#include "HVM.h"
#include "value.h"
#include "object.h"
#include "debug.h"
#include "compiler.h"
#include "table.h"

#include <stdarg.h>

HVM hvm;

static void init_stack() {
  hvm.top = hvm.stack;
}

static void runtime_error(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = hvm.ip - hvm.chunk->code - 1;
  int line = hvm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  init_stack();
}

void init_hvm() {
  init_stack();
  hvm.objects = NULL;
  init_table(&hvm.strings);
}

void free_hvm() {
  free_objects();
  free_table(&hvm.strings);
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

static bool isFalsey(Value value) {
  return (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int size = a->size + b->size;
  char* chars = ALLOCATE(char, size + 1);
  memcpy(chars, a->chars, a->size);
  memcpy(chars + a->size, b->chars, b->size);
  chars[size] = '\0';

  ObjString* result = take_string(chars, size);
  push(OBJ_VAL(result));
}

static InterReport execute() {

#define READ_BYTE() (*hvm.ip++)
#define READ_CONSTANT() (hvm.chunk->constants.values[READ_BYTE()])
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
    printf("\t");
    for (Value* pancake = hvm.stack; pancake < hvm.top; pancake++) {
      printf("[ ");
      print_value(*pancake);
      printf(" ]");
    }
    printf("\n");
    debug_instruction(hvm.chunk, (int)(hvm.ip - hvm.chunk->code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
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
      case OP_MULTI: {
        BINARY_OP(NUMBER_VAL, *);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(NUMBER_VAL, /);
        break;
      }
      case OP_RETURN: {
        print_value(pop());
        printf("\n");
        return INTER_OK;
      }
    }
  }

#undef READ_CONSTANT
#undef READ_BYTE

}

InterReport interpret(const char *source) {
  Chunk chunk;
  create_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTER_COMPILE_ERROR;
  }

  hvm.chunk = &chunk;
  hvm.ip = hvm.chunk->code;

  InterReport result = execute();
  free_chunk(&chunk);

  return result;
}


