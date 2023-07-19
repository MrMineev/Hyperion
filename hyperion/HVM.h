#ifndef hvm_h
#define hvm_h

#include <stdio.h>
#include <stdarg.h>

#include "object.h"
#include "chunk.h"
#include "table.h"

#define DEBUG_TRACE_EXECUTION

#define UINT8_COUNT (UINT8_MAX + 1)

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
  ObjClosure* closure;

  uint8_t* ip;
  Value* slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  size_t bytes_alloc;
  size_t next_gc_limit;

  Value stack[STACK_MAX];
  Value* top;
  ObjUpvalue* openUpvalues;
  Obj* objects;
  Table globals;
  Table strings;

  int gray_cnt;
  int gray_capacity;
  Obj** gray_stack;
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



