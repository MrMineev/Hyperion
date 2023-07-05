#ifndef hvm_h
#define hvm_h

#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "compiler.h"

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

static InterReport execute() {

#define READ_BYTE() (*hvm.ip++)
#define READ_CONSTANT() (hvm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
      double a = pop(); \
      double b = pop(); \
      push(b op a); \
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
      case OP_NEGATE: {
        push(-pop());
        break;
      }
      case OP_ADD: {
        BINARY_OP(+);
        break;
      }
      case OP_MINUS: {
        BINARY_OP(-);
        break;
      }
      case OP_MULTI: {
        BINARY_OP(*);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(/);
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

/*
InterReport interpret(Chunk *chunk) {
  hvm.chunk = chunk;
  hvm.ip = chunk->code;
  return execute();
}
*/

InterReport interpret(const char* source) {
  compile(source);
  return INTER_OK;
}

#endif



