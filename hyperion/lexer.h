#ifndef lexer_h
#define lexer_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
  // Modules & Import
  TOKEN_IMPORT, TOKEN_STD, TOKEN_MODULE,

  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,

  TOKEN_COMMA, TOKEN_DOT,
  TOKEN_MINUS, TOKEN_PLUS, TOKEN_SLASH, TOKEN_STAR,
  TOKEN_MINUSD, TOKEN_PLUSD, TOKEN_SLASHD, TOKEN_STARD,
  TOKEN_PLUS_COMMA,
  TOKEN_SEMICOLON, TOKEN_PERCENT,
  TOKEN_POWER,

  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  TOKEN_LINE,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_DOUBLE, TOKEN_INT,

  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_OR,
  TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_LET, TOKEN_WHILE,

  // Simple Functions
  TOKEN_PRINT, TOKEN_INC, TOKEN_DECR,

  ILLEGAL, TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  const char* start;
  int size;
  int line;
} Token;

void init_lexer(const char *source);
Token lex_token();

#endif
