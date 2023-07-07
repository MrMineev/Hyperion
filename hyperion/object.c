#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"
#include "HVM.h"

#define ALLOCATE_OBJ(type, objectType) (type*)allocate_object(sizeof(type), objectType)

static Obj* allocate_object(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->next = hvm.objects;
  hvm.objects = object;
  return object;
}

static ObjString* allocate_string(char* chars, int size) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->size = size;
  string->chars = chars;
  return string;
}

ObjString* take_string(char* chars, int length) {
  return allocate_string(chars, length);
}

ObjString* copy_string(const char* chars, int size) {
  char* heap_chars = ALLOCATE(char, size + 1);
  memcpy(heap_chars, chars, size);
  heap_chars[size] = '\0';
  return allocate_string(heap_chars, size);
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
  }
}

