#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "math.h"
#include "../../HVM.h"
#include "../../value.h"

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

void add_module(const char* name, Value (*f)(int, Value*)) {
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
  add_module("math:fac", fac_native_function);
  add_module("math:ceil", ceil_native_function);
  add_module("math:floor", floor_native_function);
}

