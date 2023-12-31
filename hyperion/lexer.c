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
          c == '_' || c == ':' || c == '@';
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
    case 'c':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'l': return search_keyword(2, 3, "ass", TOKEN_CLASS);
        }
      }
    case 'e':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'l': return search_keyword(2, 2, "se", TOKEN_ELSE);
        }
      }
      break;
    case 'f':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'a': return search_keyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return search_keyword(2, 1, "r", TOKEN_FOR);
        }
      }
      break;
    case 'd':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'e':
            if (lexer.current - lexer.start > 2 &&
                lexer.start[2] == 'c') {
              return search_keyword(2, 2, "cr", TOKEN_DECR);
            }
            return search_keyword(2, 1, "f", TOKEN_FUN);
        }
      }
    case 'i':
      if (lexer.current - lexer.start > 1) {
        switch(lexer.start[1]) {
          case 'n': return search_keyword(2, 1, "c", TOKEN_INC);
          case 'm': return search_keyword(2, 4, "port", TOKEN_IMPORT);
        }
      }
      return search_keyword(1, 1, "f", TOKEN_IF);
    case 'o': return search_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return search_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return search_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
      if (lexer.current - lexer.start > 1) {
        switch(lexer.start[1]) {
          case 't': return search_keyword(2, 1, "d", TOKEN_STD);
          case 'u': return search_keyword(2, 3, "per", TOKEN_SUPER);
        }
      }
      break;
    case 't':
      if (lexer.current - lexer.start > 1) {
        switch (lexer.start[1]) {
          case 'h':
            if (lexer.current - lexer.start > 2) {
              switch (lexer.start[2]) {
                case 'i': return search_keyword(3, 1, "s", TOKEN_THIS);
              }
            }
          case 'r': return search_keyword(2, 2, "ue", TOKEN_TRUE);
        }
      }
      break;
    case 'm': return search_keyword(1, 5, "odule", TOKEN_MODULE);
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
    return create_token(TOKEN_DOUBLE);
  }
  return create_token(TOKEN_INT);
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
    case '^': return create_token(TOKEN_POWER);
    case '(': return create_token(TOKEN_LEFT_PAREN);
    case ')': return create_token(TOKEN_RIGHT_PAREN);
    case '{': return create_token(TOKEN_LEFT_BRACE);
    case '}': return create_token(TOKEN_RIGHT_BRACE);
    case ';': return create_token(TOKEN_SEMICOLON);
    case ',': return create_token(TOKEN_COMMA);
    case '.': return create_token(TOKEN_DOT);
    case '-':
      return create_token(
        match('.') ? TOKEN_MINUSD : TOKEN_MINUS
      );
    case '+':
      return create_token(
        match('.') ? TOKEN_PLUSD : 
          (match(',') ? TOKEN_PLUS_COMMA : TOKEN_PLUS)
      );
    case '/':
      return create_token(
        match('.') ? TOKEN_SLASHD : TOKEN_SLASH
      );
    case '*':
      return create_token(
        match('.') ? TOKEN_STARD : TOKEN_STAR
      );
    case '%': return create_token(TOKEN_PERCENT);
    case '[': return create_token(TOKEN_LEFT_BRACKET);
    case ']': return create_token(TOKEN_RIGHT_BRACKET);
    case '|': return create_token(TOKEN_LINE);
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
