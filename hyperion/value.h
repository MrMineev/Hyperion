#ifndef value_h
#define value_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  VAL_BOOL,
  VAL_NUMBER,
  VAL_OBJ,
  VAL_NIL,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj *obj;
  } as; 
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})

typedef struct {
  int size;
  int capacity;
  Value* values;
} ValueArray;

void create_value_array(ValueArray *arr);
void write_value_array(ValueArray *arr, Value byte);
void free_value_array(ValueArray *arr);
void print_value(Value v);
bool are_equal(Value a, Value b);

#endif
