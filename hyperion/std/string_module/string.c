#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "string.h"
#include "../../HVM.h"
#include "../../value.h"
#include "../../commandline.h"
#include "../../object.h"

static Value make_string_string(char* str) {
  return 
    OBJ_VAL(
      copy_string(
        str,
        (int)strlen(str)
      )
    );
}

char* char_to_string(char c) {
    char* str = (char*)malloc(1);
    if (str == NULL) {
      perror("Memory allocation error");
      exit(EXIT_FAILURE);
    }
    str[0] = c;
    return str;
}

static Value chr_native_function(int argCount, Value *args) {
  return make_string_string(
      char_to_string((char)AS_INT(args[0]))
  );
}

static Value ord_native_function(int argCount, Value *args) {
  return INT_VAL(
    (int)AS_STRING(args[0])->chars[0]
  );
}

static Value get_native_function(int argCount, Value *args) {
  int index = ceil(
    AS_INT(
      args[1]
    )
  );
  return make_string_string(
    char_to_string(
      AS_STRING(args[0])->chars[index]
    )
  );
}

static Value string_len_native_function(int argCount, Value *args) {
  return INT_VAL(
    AS_STRING(args[0])->size
  );
}

void add_module_string(const char* name, Value (*f)(int, Value*)) {
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

void string_module_init() {
  add_module_string("string:len", string_len_native_function);
  add_module_string("string:ord", ord_native_function);
  add_module_string("string:chr", chr_native_function);
  add_module_string("string:get", get_native_function);
}

