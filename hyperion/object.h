#ifndef object_h
#define object_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "value.h"
#include "chunk.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_STRING,
  OBJ_FUNCTION,
} ObjType;

struct Obj {
  ObjType type;
  struct Obj* next;
};

typedef struct {
  Obj obj;
  int arity;
  Chunk chunk;
  ObjString *name;
} ObjFunction;

struct ObjString {
  Obj obj;
  int size;
  char* chars;
  uint32_t hash;
};

ObjFunction* create_function();

ObjString* take_string(char* chars, int size);
ObjString* copy_string(const char* chars, int length);
void print_object(Value value);

static inline bool is_obj_type(Value v, ObjType type) {
  return IS_OBJ(v) && AS_OBJ(v)->type == type;
}

#endif
