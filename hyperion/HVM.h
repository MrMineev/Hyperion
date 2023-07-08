#ifndef hvm_h
#define hvm_h

#include <stdio.h>
#include <stdarg.h>

#include "object.h"
#include "chunk.h"
#include "table.h"

#define DEBUG_TRACE_EXECUTION

#define STACK_MAX 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  Value stack[STACK_MAX];
  Value* top;
  Obj* objects;
  Table globals;
  Table strings;
} HVM;

typedef enum {
  INTER_OK,
  INTER_COMPILE_ERROR,
  INTER_RUNTIME_ERROR
} InterReport;

extern HVM hvm;

static void init_stack();

static void runtime_error(const char* format, ...);

void init_hvm();

void free_hvm();

void push(Value value);

Value pop();

static Value peek_c(int distance);

static bool isFalsey(Value value);

static void concatenate();

static InterReport execute();

InterReport interpret(const char *source);

#endif



