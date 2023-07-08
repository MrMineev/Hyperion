#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "chunk.h"
#include "HVM.h"
#include "object.h"
#include "compiler.h"
#include "debug.h"

#define DEBUG_PRINT_CODE

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

Parser parser;
Chunk* chunk_compiling;

static Chunk* get_chunk_compiling() {
  return chunk_compiling;
}

static void error_at(Token* token, const char* message) {
  if (parser.panic_mode) return;
  parser.panic_mode = true;

  fprintf(stderr, "[ERROR | %d line]", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == ILLEGAL) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->size, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error(const char* message) {
  error_at(&parser.previous, message);
}

static void error_current(const char* message) {
  error_at(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  while (true) {
    parser.current = lex_token();
    if (parser.current.type != ILLEGAL) break;
    error_current(parser.current.start);
  }
}

void consume(TokenType t, const char *message) {
  if (parser.current.type == t) {
    advance();
    return;
  }
  error_current(message);
}

static bool check(TokenType type) {
  return (parser.current.type == type);
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emit_byte(uint8_t byte) {
  write_chunk(get_chunk_compiling(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_return() {
  emit_byte(OP_RETURN);
}

static uint8_t create_constant(Value c) {
  int constant = add_constant(get_chunk_compiling(), c);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  } else {
    return (uint8_t) constant;
  }
}

static void emit_constant(Value c) {
  emit_bytes(OP_CONSTANT, create_constant(c));
}

static void end_compiler() {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    debug_chunk(get_chunk_compiling(), "code");
  }
#endif
}

static void expression();
static void statement();
static void declaration();
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

static void binary(bool can_assign) {
  TokenType operatorr = parser.previous.type;
  ParseRule* rule = get_rule(operatorr);
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (operatorr) {
    case TOKEN_BANG_EQUAL: emit_bytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL: emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER: emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS: emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL: emit_bytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS: emit_byte(OP_ADD); break;
    case TOKEN_MINUS: emit_byte(OP_MINUS); break;
    case TOKEN_STAR: emit_byte(OP_MULTI); break;
    case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
    default: return;
  }
}

static void literal(bool can_assign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_TRUE: emit_byte(OP_TRUE); break;
    default: return; // Unreachable.
  }
}

static void grouping(bool can_assign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression");
}

static void number_c(bool can_assign) {
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

static void string_c(bool can_assign) {
  emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1, parser.previous.size - 2)));
}

static void unary(bool can_assign) {
  TokenType operatorr = parser.previous.type;
  parse_precedence(PREC_UNARY);
  switch (operatorr) {
    case TOKEN_BANG: emit_byte(OP_NOT); break;
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return;
  }
}


static uint8_t identifier_constant(Token* name) {
  return create_constant(OBJ_VAL(copy_string(name->start, name->size)));
}

static void named_variable(Token name, bool can_assign) {
  uint8_t arg = identifier_constant(&name);
  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_GLOBAL, arg);
  } else {
    emit_bytes(OP_GET_GLOBAL, arg);
  }
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static uint8_t parse_variable(const char *error_message) {
  consume(TOKEN_IDENTIFIER, error_message);
  return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string_c, NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number_c, NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_LET]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [ILLEGAL]             = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void parse_precedence(Precedence p) {
  advance();
  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;

  if (prefix_rule == NULL) {
    error("Expect expression.");
    return;
  }

  bool can_assign = (p <= PREC_ASSIGNMENT);
  prefix_rule(can_assign);

  while (p <= get_rule(parser.current.type)->precedence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static ParseRule* get_rule(TokenType type) {
  return &rules[type];
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT);
}

static void variable_declaration() {
  uint8_t global = parse_variable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  define_variable(global);
}

static void expression_stmt() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emit_byte(OP_POP);
}

static void print_stmt() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value");
  emit_byte(OP_PRINT);
}

static void synchronize() {
  parser.panic_mode = false;
  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_LET:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:
        ; // Do nothing.
    }
    advance();
  }
}

static void declaration() {
  if (match(TOKEN_LET)) {
    variable_declaration();
  } else {
    statement();
  }

  if (parser.panic_mode) synchronize();
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_stmt();
  } else {
    expression_stmt();
  }
}

bool compile(const char* source, Chunk *chunk) {
  init_lexer(source);
  chunk_compiling = chunk;

  parser.had_error = false;
  parser.panic_mode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  end_compiler();
  return !parser.had_error;
}


