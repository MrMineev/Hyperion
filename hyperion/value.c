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
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NIL(value)) {
    printf("nil");
  } else if (IS_NUMBER(value)) {
    printf("%g", AS_NUMBER(value));
  } else if (IS_OBJ(value)) {
    print_object(value);
  }
#else
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
    case VAL_NIL:
      printf("nil");
      break;
  }
#endif
}

bool are_equal(Value a, Value b) {
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
    case VAL_NIL: return true;
    default: return false;
  }
#endif
}


