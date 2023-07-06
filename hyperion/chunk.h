#ifndef chunk_h
#define chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "memory.h"
#include "value.h"

typedef enum {
  OP_RETURN,
  OP_CONSTANT,
  OP_TRUE,
  OP_FALSE,
  OP_NOT,
  OP_NEGATE,
  OP_ADD,
  OP_MINUS,
  OP_MULTI,
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


void create_chunk(Chunk *chunk) {
  chunk->size = 0;
  chunk->capacity = 0;
  chunk->code = nullptr;
  chunk->lines = nullptr;
  create_value_array(&chunk->constants);
}

void write_chunk(Chunk *chunk, uint8_t byte, int line) {
  if (chunk->size + 1 > chunk->capacity) {
    int capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, capacity, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, capacity, chunk->capacity);
  }
  chunk->code[chunk->size] = byte;
  chunk->lines[chunk->size] = line;
  chunk->size++;
}

int add_constant(Chunk* chunk, Value value) {
  write_value_array(&chunk->constants, value);
  return chunk->constants.size - 1;
}

void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->capacity);
  free_value_array(&chunk->constants);
  create_chunk(chunk);
}

#endif
