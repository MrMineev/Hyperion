#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "math.h"
#include "../../HVM.h"
#include "../../value.h"

static Value pi_native_function(int argCount, Value *args) {
  double PI = 3.14159265;
  return NUMBER_VAL(PI);
}

static Value cos_native_function(int argCount, Value *args) {
    double angle_radians = AS_NUMBER(args[0]);
    double result = cos(angle_radians);
    return NUMBER_VAL(result);
}

static Value sin_native_function(int argCount, Value *args) {
    double angle_radians = AS_NUMBER(args[0]);
    double result = sin(angle_radians);
    return NUMBER_VAL(result);
}

static Value abs_native_function(int argCount, Value* args) {
  double c = AS_NUMBER(args[0]);
  if (c < 0) {
    return NUMBER_VAL(-c);
  } else {
    return NUMBER_VAL(c);
  }
}

static Value fac_native_function(int argCount, Value* args) {
  int r = 1;
  for (int i = 1; i <= (int)AS_NUMBER(args[0]); i++) {
    r *= i;
  }
  return NUMBER_VAL((double)r);
}

static Value ceil_native_function(int argCount, Value* args) {
  int r = ceil(AS_NUMBER(args[0]));
  return NUMBER_VAL((double)r);
}

static Value floor_native_function(int argCount, Value* args) {
  int r = floor(AS_NUMBER(args[0]));
  return NUMBER_VAL((double)r);
}

void add_module_math(const char* name, Value (*f)(int, Value*)) {
  set_table(
      &hvm.globals,
      AS_STRING(
        OBJ_VAL(
          copy_string(name, (int)strlen(name))
        )
      ),
      OBJ_VAL(
        create_native(f)
      )
  );
}

void math_module_init() {
  add_module_math("math:fac", fac_native_function);
  add_module_math("math:ceil", ceil_native_function);
  add_module_math("math:floor", floor_native_function);
  add_module_math("math:abs", abs_native_function);
  add_module_math("math:sin", sin_native_function);
  add_module_math("math:cos", cos_native_function);
  add_module_math("math:pi", pi_native_function);
}

