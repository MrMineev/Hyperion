#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "math.h"
#include "../../HVM.h"
#include "../../value.h"


static Value atan2_native_function(int argCount, Value *args) {
  double y = AS_DOUBLE(args[0]);
  double x = AS_DOUBLE(args[1]);
  return DOUBLE_VAL(atan2(x, y));
}


static Value power_native_function(int argCount, Value *args) {
  double x = AS_DOUBLE(args[0]);
  double y = AS_DOUBLE(args[1]);

  double power = pow(x, y);

  return DOUBLE_VAL(power);
}

static Value to_radians_native_function(int argCount, Value *args) {
  double PI = 3.14159265;
  double deg = AS_DOUBLE(args[0]);
  return DOUBLE_VAL(
      deg * PI / 180.0
  );
}

static Value e_native_function(int argCount, Value *args) {
  double e = 2.718281828;
  return DOUBLE_VAL(e);
}

static Value golden_ratio_native_function(int argCount, Value *args) {
  double ratio = 1.6180339887498948;
  return DOUBLE_VAL(ratio);
}

static Value pi_native_function(int argCount, Value *args) {
  double PI = 3.14159265;
  return DOUBLE_VAL(PI);
}

static Value cos_native_function(int argCount, Value *args) {
    double angle_radians = AS_DOUBLE(args[0]);
    double result = cos(angle_radians);
    return DOUBLE_VAL(result);
}

static Value tan_native_function(int argCount, Value *args) {
  double angle_radians = AS_DOUBLE(args[0]);
  double result = tan(angle_radians);
  return DOUBLE_VAL(result);
}

static Value sin_native_function(int argCount, Value *args) {
    double angle_radians = AS_DOUBLE(args[0]);
    double result = sin(angle_radians);
    return DOUBLE_VAL(result);
}

static Value abs_native_function(int argCount, Value* args) {
  if (IS_DOUBLE(args[0])) {
    double c = AS_DOUBLE(args[0]);
    if (c < 0) {
      return DOUBLE_VAL(-c);
    } else {
      return DOUBLE_VAL(c);
    }
  } else if (IS_INT(args[0])) {
    int c = AS_INT(args[0]);
    if (c < 0) {
      return INT_VAL(-c);
    } else {
      return INT_VAL(c);
    }
  } else {
    return NIL_VAL;
  }
}

static Value fac_native_function(int argCount, Value* args) {
  int r = 1;
  for (int i = 1; i <= AS_INT(args[0]); i++) {
    r *= i;
  }
  return INT_VAL(r);
}

static Value ceil_native_function(int argCount, Value* args) {
  int r = ceil(AS_DOUBLE(args[0]));
  return INT_VAL(r);
}

static Value floor_native_function(int argCount, Value* args) {
  int r = floor(AS_DOUBLE(args[0]));
  return DOUBLE_VAL((double)r);
}

int gcd(int a, int b) {
  if (b == 0) return a;
  return gcd(b, a % b);
}

static Value gcd_native_function(int argCount, Value *args) {
  int n = AS_INT(args[0]);
  int m = AS_INT(args[1]);
  return INT_VAL(gcd(n, m));
}

static Value lcm_native_function(int argCount, Value *args) {
  int n = AS_INT(args[0]);
  int m = AS_INT(args[1]);
  return INT_VAL(
    n * m / gcd(n, m)
  );
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
  add_module_math("math:gcd", gcd_native_function);
  add_module_math("math:lcm", lcm_native_function);
  add_module_math("math:fac", fac_native_function);
  add_module_math("math:ceil", ceil_native_function);
  add_module_math("math:floor", floor_native_function);
  add_module_math("math:abs", abs_native_function);
  add_module_math("math:sin", sin_native_function);
  add_module_math("math:cos", cos_native_function);
  add_module_math("math:tan", tan_native_function);
  add_module_math("math:atan2", atan2_native_function);
  add_module_math("math:pi", pi_native_function);
  add_module_math("math:to_radians", to_radians_native_function);
  add_module_math("math:e", e_native_function);
  add_module_math("math:golden_ratio", golden_ratio_native_function);
  add_module_math("math:pow", power_native_function);
}

