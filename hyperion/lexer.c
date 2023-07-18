#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

typedef struct {
  const char *start;
  const char *current;
  int line;
} Lexer;

Lexer lexer;

void init_lexer(const char *source) {
  lexer.start = source;
  lexer.current = source;
  lexer.line = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool is_eof() {
  return *lexer.current == '\0';
}

static char read_char() {
  lexer.current++;
  return lexer.current[-1];
}

static char peek() {
  return *lexer.current;
}

static char next_peek() {
  if (is_eof()) return '\0';
  return lexer.current[1];
}

static bool match(char expected) {
  if (is_eof()) return false;
  if (*lexer.current != expected) return false;
  lexer.current++;
  return true;
}

static Token create_token(TokenType type) {
  Token t;
  t.type = type;
  t.start = lexer.start;
  t.size = (int)(lexer.current - lexer.start);
  t.line = lexer.line;
  return t;
}

static Token error_token(const char *message) {
  Token t;
  t.type = ILLEGAL;
  t.start = message;
  t.size = (int)strlen(message);
  t.line = lexer.line;
  return t;
}

void skip_whitespace() {
  while (true) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        read_char();
        break;
      case '\n':
        lexer.line++;
        read_char();
        break;
      case '/':
        if (next_peek() == '/') {
          while (peek() != '\n' && !is_eof()) read_char();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static TokenType search_keyword(int start, int size, const char* rest, TokenType type) {
  if (lexer.current - lexer.start == start + size &&
      memcmp(lexer.start + start, rest, size) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  switch (lexer.start[0]) {
    case 'a': return search_keyword(1, 2, "nd", TOKEN_AND);
    case 'c': return search_keyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return search_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'a': return search_keyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return search_keyword(2, 1, "r", TOKEN_FOR);
          case 'u': return search_keyword(2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 'd':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'e': return search_keyword(2, 2, "cr", TOKEN_DECR);
        }
      }
    case 'i':
      if (lexer.current - lexer.start > 2 &&
          lexer.start[1] == 'n' &&
          lexer.start[2] == 'c') {
        return TOKEN_INC;
      }
      return search_keyword(1, 1, "f", TOKEN_IF);
    case 'o': return search_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return search_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return search_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return search_keyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'h': return search_keyword(2, 2, "is", TOKEN_THIS);
          case 'r': return search_keyword(2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'l': return search_keyword(1, 2, "et", TOKEN_LET);
    case 'w': return search_keyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) read_char();
  return create_token(identifierType());
}

static Token number() {
  while (isDigit(peek())) read_char();
  if (peek() == '.' && isDigit(next_peek())) {
    read_char();
    while (isDigit(peek())) read_char();
  }
  return create_token(TOKEN_NUMBER);
}

static Token string() {
  while (peek() != '"' && !is_eof()) {
    if (peek() == '\n') lexer.line++;
    read_char();
  }
  if (is_eof()) return error_token("Unterminated string.");
  read_char();
  return create_token(TOKEN_STRING);
}

Token lex_token() {
  skip_whitespace();

  lexer.start = lexer.current;
  if (is_eof()) return create_token(TOKEN_EOF);
  char c = read_char();

  if (isDigit(c)) return number();
  if (isAlpha(c)) return identifier();

  switch (c) {
    case '(': return create_token(TOKEN_LEFT_PAREN);
    case ')': return create_token(TOKEN_RIGHT_PAREN);
    case '{': return create_token(TOKEN_LEFT_BRACE);
    case '}': return create_token(TOKEN_RIGHT_BRACE);
    case ';': return create_token(TOKEN_SEMICOLON);
    case ',': return create_token(TOKEN_COMMA);
    case '.': return create_token(TOKEN_DOT);
    case '-': return create_token(TOKEN_MINUS);
    case '+': return create_token(TOKEN_PLUS);
    case '/': return create_token(TOKEN_SLASH);
    case '*': return create_token(TOKEN_STAR);
    case '%': return create_token(TOKEN_PERCENT);
    case '!':
      return create_token(
          match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return create_token(
          match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return create_token(
          match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return create_token(
          match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
  }

  return error_token("Unexpected character.");
}
