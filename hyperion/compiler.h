#ifndef compiler_h
#define compiler_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "lexer.h"

void compile(const char* source) {
  init_lexer(source);
  int line = -1;
  while (true) {
    Token t = lex_token();
    if (t.line != line) {
      printf("%4d ", t.line);
      line = t.line;
    } else {
      printf("   | ");
    }
    printf("%2d '%.*s'\n", t.type, t.size, t.start);

    if (t.type == TOKEN_EOF) {
      break;
    }
  }
}

#endif
