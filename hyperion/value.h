#ifndef value_h
#define value_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define QNAN     ((uint64_t)0x7ffc000000000000)
#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define TAG_NIL   1 // 01.
#define TAG_FALSE 2 // 10.
#define TAG_TRUE  3 // 11.

typedef uint64_t Value;

#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_NUMBER(value) value_to_num(value)
#define AS_OBJ(value) \
    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define AS_BOOL(value) ((value) == TRUE_VAL)

#define NUMBER_VAL(num) num_to_value(num)
#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))
#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double value_to_num(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

static inline Value numToValue(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

#else

typedef enum {
  VAL_BOOL,
  VAL_DOUBLE,
  VAL_INT,
  VAL_OBJ,
  VAL_NIL,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool Boolean;
    double Double;
    int Integer;
    Obj *obj;
  } as; 
} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_DOUBLE(value)  ((value).type == VAL_DOUBLE)
#define IS_INT(value)     ((value).type == VAL_INT)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_BOOL(value)    ((value).as.Boolean)
#define AS_DOUBLE(value)  ((value).as.Double)
#define AS_INT(value)     ((value).as.Integer)
#define AS_OBJ(value)     ((value).as.obj)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.Boolean = value}})
#define DOUBLE_VAL(value) ((Value){VAL_DOUBLE, {.Double = value}})
#define INT_VAL(value)    ((Value){VAL_INT, {.Integer = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})
#define NIL_VAL           ((Value){VAL_NIL, {.Double = 0}})

#endif

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
