#ifndef chunk_h
#define chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory.h"

typedef enum {
  OP_RETURN
} Commands;

typedef struct {
  int size;
  int capacity;
  uint8_t *code;
} Chunk;


void create_chunk(Chunk *chunk) {
  chunk->size = 0;
  chunk->capacity = 0;
  chunk->code = nullptr;
}

void write_chunk(Chunk *chunk, uint8_t byte) {
  if (chunk->size + 1 > chunk->capacity) {
    int capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, capacity, chunk->capacity);
  }
  chunk->code[chunk->size] = byte;
  chunk->size++;
}

void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  create_chunk(chunk);
}

#endif
