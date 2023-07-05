#ifndef debug_h
#define debug_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "chunk.h"

int simple_instruction(const char* op_command, int offset) {
  printf("%s\n", op_command);
  return offset + 1;
}

int debug_instruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

void debug_chunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->size;) {
    offset = debug_instruction(chunk, offset);
  }
}

#endif
