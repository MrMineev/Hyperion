#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"
#include "table.h"
#include "HVM.h"

#define ALLOCATE_OBJ(type, objectType) (type*)allocate_object(sizeof(type), objectType)

static Obj* allocate_object(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->next = hvm.objects;
  hvm.objects = object;
  return object;
}

static ObjString* allocate_string(char* chars, int size, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->size = size;
  string->chars = chars;
  string->hash = hash;
  set_table(&hvm.strings, string, NIL_VAL);
  return string;
}

static uint32_t hash_string(const char *key, int size) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < size; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString* take_string(char *chars, int size) {
  uint32_t hash = hash_string(chars, size);
  ObjString* interned = table_find_string(&hvm.strings, chars, size, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, size + 1);
    return interned;
  }
  return allocate_string(chars, size, hash);
}

ObjString* copy_string(const char* chars, int size) {
  uint32_t hash = hash_string(chars, size);

  ObjString* interned = table_find_string(&hvm.strings, chars, size, hash);
  if (interned != NULL) return interned;

  char* heap_chars = ALLOCATE(char, size + 1);
  memcpy(heap_chars, chars, size);
  heap_chars[size] = '\0';
  return allocate_string(heap_chars, size, hash);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}

