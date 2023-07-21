#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "type_conversion.h"
#include "../../HVM.h"
#include "../../value.h"
#include "../../object.h"

static Value make_string(char* str) {
  return 
    OBJ_VAL(
      copy_string(
        str,
        (int)strlen(str)
      )
    );
}


static Value to_string_native_function(int argCount, Value* args) {
  if (IS_NUMBER(args[0])) {
    double c = AS_NUMBER(args[0]);
    char str[100];
    snprintf(str, 10, "%f", c);

    return make_string(str);
  } else if (IS_BOOL(args[0])) {
    if (AS_BOOL(args[0]) == true) {
      return make_string((char*)"true");
    } else if (AS_BOOL(args[0]) == false) {
      return make_string((char*)"false");
    } else {
      return make_string((char*)"ERROR");
    }
  } else {
    return make_string((char*)"ERROR");
  }
}

void add_module_type_conv(const char* name, Value (*f)(int, Value*)) {
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

void type_conversion_module_init() {
  add_module_type_conv("type_conv:to_string", to_string_native_function);
}

