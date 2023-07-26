#ifndef compiler_h
#define compiler_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "chunk.h"
#include "HVM.h"
#include "object.h"

ObjFunction* compile(const char* source);
void mark_compiler_roots();
static uint8_t create_constant(Value c);

#endif
