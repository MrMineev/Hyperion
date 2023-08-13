#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "random.h"
#include "../../HVM.h"
#include "../../value.h"
#include "../../commandline.h"
#include "../../object.h"

static Value make_string_os(char* str) {
  return 
    OBJ_VAL(
      copy_string(
        str,
        (int)strlen(str)
      )
    );
}

static Value rand_native_function(int argCount, Value *args) {
  return INT_VAL(rand());
}

static Value srand_native_function(int argCount, Value *args) {
  srand(AS_INT(args[0]));
  return NIL_VAL;
}

void add_module_random(const char* name, Value (*f)(int, Value*)) {
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

void random_module_init() {
  add_module_random("random:rand", rand_native_function);
  add_module_random("random:srand", srand_native_function);
}

