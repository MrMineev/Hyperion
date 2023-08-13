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

static Value to_double_native_function(int argCount, Value *args) {
  if (IS_DOUBLE(args[0])) {
    return args[0];
  } else if (IS_INT(args[0])) {
    return DOUBLE_VAL((double)AS_INT(args[0]));
  } else {
    return NIL_VAL;
  }
}

static Value to_string_native_function(int argCount, Value* args) {
  if (IS_DOUBLE(args[0])) {
    double c = AS_DOUBLE(args[0]);
    char str[100];
    int len = snprintf(str, 10, "%f", c);

    if (len > 0) {
      int i = len - 1;
      while (i >= 0 && str[i] == '0') {
        str[i] = '\0';
        i--;
      }
      if (i >= 0 && str[i] == '.') {
        str[i] = '\0';
      }
    }

    return make_string(str);
  } else if (IS_INT(args[0])) {
    int c = AS_INT(args[0]);
    char str[100];
    int len = snprintf(str, 10, "%i", c);

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
  add_module_type_conv("type_conv:to_double", to_double_native_function);
}

