#ifndef value_h
#define value_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
} ValueType;

typedef typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as; 
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

typedef struct {
  int size;
  int capacity;
  Value* values;
} ValueArray;

void create_value_array(ValueArray *arr) {
  arr->size = 0;
  arr->capacity = 0;
  arr->values = nullptr;
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
    case VAL_NIL: return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
    default: return false;
  }
}

#endif
