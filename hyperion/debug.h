#ifndef debug_h
#define debug_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "chunk.h"

int simple_instruction(const char* op_command, int offset);
int constant_instruction(const char* op_command, Chunk *chunk, int offset);
int debug_instruction(Chunk *chunk, int offset);
void debug_chunk(Chunk* chunk, const char* name);

#endif
