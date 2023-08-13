#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "math.h"
#include "../../HVM.h"
#include "../../value.h"

static Value make_string_console(char* str) {
  return 
    OBJ_VAL(
      copy_string(
        str,
        (int)strlen(str)
      )
    );
}

static Value get_line_native_function(int argCount, Value* args) {
  char* input = NULL;
  size_t len;

  ssize_t read = getline(&input, &len, stdin);

  if (read != -1) {
    return make_string_console(input);
  } else {
    return NIL_VAL;
  }
}

static Value floor_native_function(int argCount, Value* args) {
  int r = floor(AS_DOUBLE(args[0]));
  return INT_VAL(r);
}

void add_module_console(const char* name, Value (*f)(int, Value*)) {
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

void console_module_init() {
  add_module_console("console:get_line", get_line_native_function);
}

