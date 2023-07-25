#ifndef chunk_h
#define chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "value.h"

typedef enum {
  OP_BUILD_LIST,
  OP_INDEX_SUBSCR,
  OP_STORE_SUBSCR,
  OP_POP,
  OP_IMPORT_STD,
  OP_IMPORT_MODULE,
  OP_INVOKE,
  OP_METHOD,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_CLASS,
  OP_CLOSURE,
  OP_PRINT,
  OP_PRINT_TOLINE,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_JUMP,
  OP_CALL,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_CLOSE_UPVALUE,
  OP_NIL,
  OP_RETURN,
  OP_CONSTANT,
  OP_TRUE,
  OP_FALSE,
  OP_NOT,
  OP_NEGATE,
  OP_ADD,
  OP_MINUS,
  OP_MULTI,
  OP_MODULE,
  OP_DIVIDE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS
} Commands;

typedef struct {
  int size;
  int capacity;
  uint8_t *code;
  ValueArray constants;
  int *lines;
} Chunk;


void create_chunk(Chunk *chunk);

void write_chunk(Chunk *chunk, uint8_t byte, int line);

int add_constant(Chunk* chunk, Value value);

void free_chunk(Chunk *chunk);

#endif
