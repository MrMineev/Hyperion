#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "chunk.h"
#include "HVM.h"
#include "object.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"

#define UINT8_COUNT (UINT8_MAX + 1)

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
  Token name;
  int depth;
  bool isCaptured;
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT, 
  TYPE_METHOD, 
  TYPE_INITIALIZER
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing;

  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int local_count;
  Upvalue upvalues[UINT8_COUNT];
  int scope_depth;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
} ClassCompiler;

typedef struct {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
} Parser;

Compiler *current = nullptr;
ClassCompiler* current_class = NULL;
Parser parser;

static Chunk* get_chunk_compiling() {
  return &current->function->chunk;
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

static void emit_loop(int loop_start) {
  emit_byte(OP_LOOP);

  int offset = get_chunk_compiling()->size - loop_start + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return get_chunk_compiling()->size - 2;
}

static void emit_return() {
  if (current->type == TYPE_INITIALIZER) {
    emit_bytes(OP_GET_LOCAL, 0);
  } else {
    emit_byte(OP_NIL);
  }

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

static void patch_jump(int offset) {
  int jump = get_chunk_compiling()->size - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  get_chunk_compiling()->code[offset] = (jump >> 8) & 0xff;
  get_chunk_compiling()->code[offset + 1] = jump & 0xff;
}

void init_compiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current;

  compiler->function = NULL;
  compiler->type = type;

  compiler->local_count = 0;
  compiler->scope_depth = 0;

  compiler->function = create_function();

  current = compiler;

  if (type != TYPE_SCRIPT) {
    current->function->name = copy_string(parser.previous.start, parser.previous.size);
  }

  Local* local = &current->locals[current->local_count++];
  local->depth = 0;
  local->isCaptured = false;

  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.size = 4;
  } else {
    local->name.start = "";
    local->name.size = 0;
  }
}

static ObjFunction* end_compiler() {
  emit_return();
  ObjFunction* function = current->function;
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    debug_chunk(get_chunk_compiling(), function->name != NULL
        ? function->name->chars : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}

static void init_scope() {
  current->scope_depth++;
}

static void destroy_scope() {
  current->scope_depth--;

  while (current->local_count > 0 &&
         current->locals[current->local_count - 1].depth > current->scope_depth) {
    if (current->locals[current->local_count - 1].isCaptured) {
      emit_byte(OP_CLOSE_UPVALUE);
    } else {
      emit_byte(OP_POP);
    }
    current->local_count--;
  }
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
    case TOKEN_PERCENT: emit_byte(OP_MODULE); break;
    default: return;
  }
}

static uint8_t identifier_constant(Token* name) {
  return create_constant(OBJ_VAL(copy_string(name->start, name->size)));
}

static uint8_t argument_list();

static void call(bool can_assign) {
  uint8_t cnt = argument_list();
  emit_bytes(OP_CALL, cnt);
}

static void dot(bool can_assign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifier_constant(&parser.previous);

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_PROPERTY, name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t cnt = argument_list();
    emit_bytes(OP_INVOKE, name);
    emit_byte(cnt);
  } else {
    emit_bytes(OP_GET_PROPERTY, name);
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

static bool identifiers_equal(Token* a, Token* b) {
  if (a->size != b->size) return false;
  return memcmp(a->start, b->start, a->size) == 0;
}

static int resolve_local(Compiler* compiler, Token* name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local) {
  int upvalue_cnt = compiler->function->upvalueCount;

  for (int i = 0; i < upvalue_cnt; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == is_local) {
      return i;
    }
  }

  if (upvalue_cnt == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalue_cnt].isLocal = is_local;
  compiler->upvalues[upvalue_cnt].index = index;
  return compiler->function->upvalueCount++;
}

static int resolve_upvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) {
    return -1;
  }

  int local = resolve_local(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return add_upvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolve_upvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return add_upvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

static void add_local_variable(Token name) {
  if (current->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

static void declare_variable() {
  if (current->scope_depth == 0) return;

  Token* name = &parser.previous;
  for (int i = current->local_count - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scope_depth) {
      break; 
    }

    if (identifiers_equal(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
  add_local_variable(*name);
}

static void named_variable(Token name, bool can_assign) {
  uint8_t getOp, setOp;
  int arg = resolve_local(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolve_upvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifier_constant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(setOp, (uint8_t)arg);
  } else {
    emit_bytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void this_(bool canAssign) {
  if (current_class == NULL) {
    error("Can't use 'this' outside of a class.");
    return;
  }

  variable(false);
}

static void mark_initialized() {
  if (current->scope_depth == 0) return;
  current->locals[current->local_count - 1].depth = current->scope_depth;
}

static uint8_t parse_variable(const char *error_message) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable();
  if (current->scope_depth > 0) return 0;

  return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
  if (current->scope_depth > 0) {
    mark_initialized();
    return;
  }
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argument_list() {
  uint8_t cnt = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (cnt == 255) {
        error("Can't have more than 255 arguments.");
      }
      cnt++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return cnt;
}

static void and_(bool canAssign) {
  int end_jump = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP);
  parse_precedence(PREC_AND);

  patch_jump(end_jump);
}

static void or_(bool canAssign) {
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);

  patch_jump(else_jump);
  emit_byte(OP_POP);

  parse_precedence(PREC_OR);
  patch_jump(end_jump);
}

ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_PERCENT]       = {unary,    binary, PREC_TERM},
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
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
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

static void inc_stmt();
static void decr_stmt();

static void expression() {
  if (match(TOKEN_INC)) {
    inc_stmt();
    return;
  } else if (match(TOKEN_DECR)) {
    decr_stmt();
    return;
  }
  parse_precedence(PREC_ASSIGNMENT);
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type) {
  Compiler compiler;
  init_compiler(&compiler, type);
  init_scope(); 

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        error_current("Can't have more than 255 parameters.");
      }
      uint8_t constant = parse_variable("Expect parameter name.");
      define_variable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction* function = end_compiler();
  emit_bytes(OP_CLOSURE, create_constant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emit_byte(compiler.upvalues[i].isLocal ? 1 : 0);
    emit_byte(compiler.upvalues[i].index);
  }
}

static void method() {
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifier_constant(&parser.previous);

  FunctionType type = TYPE_METHOD;
  if (parser.previous.size == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
    type = TYPE_INITIALIZER;
  }

  function(type);

  emit_bytes(OP_METHOD, constant);
}

static void class_declaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  Token t_class_name = parser.previous;
  uint8_t class_name = identifier_constant(&parser.previous);
  declare_variable();

  emit_bytes(OP_CLASS, class_name);
  define_variable(class_name);

  ClassCompiler class_compiler;
  class_compiler.enclosing = current_class;
  current_class = &class_compiler;

  named_variable(t_class_name, false);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");

  emit_byte(OP_POP);

  current_class = current_class->enclosing;
}

static void function_declaration() {
  uint8_t global = parse_variable("Expect function name.");
  mark_initialized();
  function(TYPE_FUNCTION);
  define_variable(global);
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

static void for_statement() {
  init_scope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_LET)) {
    variable_declaration();
  } else {
    expression_stmt();
  }

  int loop_start = get_chunk_compiling()->size;
  int exit_jump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = get_chunk_compiling()->size;
    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement();
  emit_loop(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP); // Condition.
  }

  destroy_scope();
}

static void if_statement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();

  int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);

  if (match(TOKEN_ELSE)) {
    statement();
  }
  patch_jump(else_jump);
}

static void print_stmt() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value");
  emit_byte(OP_PRINT);
}

static void return_stmt() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emit_return();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }

    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emit_byte(OP_RETURN);
  }
}

static void while_statement() {
  int loop_start = get_chunk_compiling()->size;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();

  emit_loop(loop_start);

  patch_jump(exit_jump);
  emit_byte(OP_POP);
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
  if (match(TOKEN_CLASS)) {
    class_declaration();
  } else if (match(TOKEN_FUN)) {
    function_declaration();
  } else if (match(TOKEN_LET)) {
    variable_declaration();
  } else {
    statement();
  }

  if (parser.panic_mode) synchronize();
}

static void decr_stmt() {
  if (match(TOKEN_IDENTIFIER)) {
    uint8_t getOp, setOp;
    int arg = resolve_local(current, &parser.previous);
    if (arg != -1) {
      getOp = OP_GET_LOCAL;
      setOp = OP_SET_LOCAL;
    } else if ((arg = resolve_upvalue(current, &parser.previous)) != -1) {
      getOp = OP_GET_UPVALUE;
      setOp = OP_SET_UPVALUE;
    } else {
      arg = identifier_constant(&parser.previous);
      getOp = OP_GET_GLOBAL;
      setOp = OP_SET_GLOBAL;
    }

    emit_bytes(getOp, arg);
    emit_bytes(OP_CONSTANT, create_constant(NUMBER_VAL(-1)));
    emit_byte(OP_ADD);
    emit_bytes(setOp, arg);
  } else {
    error("Expect variable name.");
  }
}

static void inc_stmt() {
  if (match(TOKEN_IDENTIFIER)) {
    uint8_t getOp, setOp;
    int arg = resolve_local(current, &parser.previous);
    if (arg != -1) {
      getOp = OP_GET_LOCAL;
      setOp = OP_SET_LOCAL;
    } else if ((arg = resolve_upvalue(current, &parser.previous)) != -1) {
      getOp = OP_GET_UPVALUE;
      setOp = OP_SET_UPVALUE;
    } else {
      arg = identifier_constant(&parser.previous);
      getOp = OP_GET_GLOBAL;
      setOp = OP_SET_GLOBAL;
    }

    emit_bytes(getOp, arg);
    emit_bytes(OP_CONSTANT, create_constant(NUMBER_VAL(1)));
    emit_byte(OP_ADD);
    emit_bytes(setOp, arg);
  } else {
    error("Expect variable name.");
  }

  /*
  uint8_t global = parse_variable("Expect variable name.");

  emit_bytes(OP_GET_GLOBAL, global);
  emit_bytes(OP_CONSTANT, create_constant(NUMBER_VAL(1)));
  emit_byte(OP_ADD);
  emit_bytes(OP_SET_GLOBAL, global);
  */
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_stmt();
  } else if (match(TOKEN_FOR)) {
    for_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_RETURN)) {
    return_stmt();
  } else if (match(TOKEN_LEFT_BRACE)) {
    init_scope();
    block();
    destroy_scope();
  } else {
    expression_stmt();
  }
}

ObjFunction* compile(const char* source) {
  init_lexer(source);

  Compiler compiler;
  init_compiler(&compiler, TYPE_SCRIPT);

  parser.had_error = false;
  parser.panic_mode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction *function = end_compiler();
  return parser.had_error ? NULL : function;
}

void mark_compiler_roots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    mark_object_memory((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}


