#ifndef hvm_h
#define hvm_h

#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "compiler.h"
#include <stdarg.h>

#define DEBUG_TRACE_EXECUTION

#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value* top;
} HVM;

typedef enum {
  INTER_OK,
  INTER_COMPILE_ERROR,
  INTER_RUNTIME_ERROR
} InterReport;

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
}

void free_hvm() {
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
        BINARY_OP(NUMBER_VAL, +);
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

/*

InterReport interpret(const char* source) {
  compile(source);
  return INTER_OK;
}

*/

#endif



