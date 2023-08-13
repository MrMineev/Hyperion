#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "chunk.h"
#include "debug.h"
#include "object.h"

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

static int invoke_instruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  uint8_t cnt = chunk->code[offset + 2];
  printf("%-16s (%d args) %4d '", name, cnt, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

static int byte_instruction(const char* name, Chunk* chunk, int offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2; 
}

static int jump_instruction(const char* name, int sign, Chunk* chunk, int offset) {
  uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
  jump |= chunk->code[offset + 2];
  printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
  return offset + 3;
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
    case OP_BUILD_LIST:
      return byte_instruction("OP_BUILD_LIST", chunk, offset);
    case OP_INDEX_SUBSCR:
      return simple_instruction("OP_INDEX_SUBSCR", offset);
    case OP_STORE_SUBSCR:
      return simple_instruction("OP_STORE_SUBSCR", offset);
    case OP_IMPORT_STD:
      return constant_instruction("OP_IMPORT_STD", chunk, offset);
    case OP_IMPORT_MODULE:
      return constant_instruction("OP_IMPORT_MODULE", chunk, offset);
    case OP_INVOKE:
      return invoke_instruction("OP_INVOKE", chunk, offset);
    case OP_METHOD:
      return constant_instruction("OP_METHOD", chunk, offset);
    case OP_GET_PROPERTY:
      return constant_instruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
      return constant_instruction("OP_SET_PROPERTY", chunk, offset);
    case OP_CLASS:
      return constant_instruction("OP_CLASS", chunk, offset);
    case OP_NIL:
      return simple_instruction("OP_NIL", offset);
    case OP_LOOP:
      return jump_instruction("OP_LOOP", -1, chunk, offset);
    case OP_JUMP:
      return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    case OP_PRINT_TOLINE:
      return simple_instruction("OP_PRINT_TOLINE", offset);
    case OP_PRINT:
      return simple_instruction("OP_PRINT", offset);
    case OP_POP:
      return simple_instruction("OP_POP", offset);
    case OP_CALL:
      return byte_instruction("OP_CALL", chunk, offset);
    case OP_GET_LOCAL:
      return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byte_instruction("OP_SET_LOCAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
      return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_POWER:
      return simple_instruction("OP_POWER", offset);
    case OP_MODULE:
      return simple_instruction("OP_MODULE", offset);
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_MINUS:
      return simple_instruction("OP_MINUS", offset);
    case OP_MULTI:
      return simple_instruction("OP_MULTI", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_ADD_D:
      return simple_instruction("OP_ADD_D", offset);
    case OP_ADD_S:
      return simple_instruction("OP_ADD_S", offset);
    case OP_MINUS_D:
      return simple_instruction("OP_MINUS_D", offset);
    case OP_MULTI_D:
      return simple_instruction("OP_MULTI_D", offset);
    case OP_DIVIDE_D:
      return simple_instruction("OP_DIVIDE_D", offset);
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
    case OP_GET_UPVALUE:
      return byte_instruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byte_instruction("OP_SET_UPVALUE", chunk, offset);
    case OP_CLOSE_UPVALUE:
      return simple_instruction("OP_CLOSE_UPVALUE", offset);
    case OP_CLOSURE: {
      offset++;
      uint8_t constant = chunk->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      print_value(chunk->constants.values[constant]);
      printf("\n");

      ObjFunction* function = AS_FUNCTION(chunk->constants.values[constant]);
      for (int j = 0; j < function->upvalueCount; j++) {
        int isLocal = chunk->code[offset++];
        int index = chunk->code[offset++];
        printf(
            "%04d      |                     %s %d\n",
            offset - 2, isLocal ? "local" : "upvalue", index
        );
      }
      return offset;
    }
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

