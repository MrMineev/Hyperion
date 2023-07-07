#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "memory.h"
#include "value.h"
#include "chunk.h"

void create_chunk(Chunk *chunk) {
  chunk->size = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
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


