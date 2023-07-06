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

int constant_instruction(const char* op_command, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", op_command, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int debug_instruction(Chunk *chunk, int offset) {
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_MINUS:
      return simple_instruction("OP_MINUS", offset);
    case OP_MULTI:
      return simple_instruction("OP_MULTI", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_CONSTANT:
      return constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_TRUE:
      return simple_instruction("OP_TRUE", offset);
    case OP_FALSE:
      return simple_instruction("OP_FALSE", offset);
    case OP_NOT:
      return simple_instruction("OP_NOT", offset);
    case OP_EQUAL:
      return simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simple_instruction("OP_GREATER", offset);
    case OP_LESS:
      return simple_instruction("OP_LESS", offset);
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
