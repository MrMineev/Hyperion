#ifndef value_h
#define value_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef double Value;

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
  printf("%g", v);
}

#endif
