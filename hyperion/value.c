#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

void create_value_array(ValueArray *arr) {
  arr->size = 0;
  arr->capacity = 0;
  arr->values = NULL;
}

void write_value_array(ValueArray *arr, Value byte) {
  if (arr->size + 1 > arr->capacity) {
    int capacity = arr->capacity;
    arr->capacity = GROW_CAPACITY(capacity);
    arr->values = GROW_ARRAY(Value, arr->values, capacity, arr->capacity);
  }
  arr->values[arr->size] = byte;
  arr->size++;
}

void free_value_array(ValueArray *arr) {
  FREE_ARRAY(Value, arr->values, arr->capacity);
  create_value_array(arr);
}

void print_value(Value v) {
  switch (v.type) {
    case VAL_OBJ:
      print_object(v);
      break;
    case VAL_BOOL:
      printf(AS_BOOL(v) ? "true" : "false");
      break;
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(v));
      break;
  }
}

bool are_equal(Value a, Value b) {
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: {
      ObjString* aString = AS_STRING(a);
      ObjString* bString = AS_STRING(b);
      return aString->size == bString->size &&
          memcmp(aString->chars, bString->chars, aString->size) == 0;
    }
    default: return false;
  }
}


