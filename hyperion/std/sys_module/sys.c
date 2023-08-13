#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "sys.h"
#include "../../HVM.h"
#include "../../value.h"
#include "../../commandline.h"
#include "../../object.h"

static Value make_string_sys(char* str) {
  return 
    OBJ_VAL(
      copy_string(
        str,
        (int)strlen(str)
      )
    );
}

static Value get_argv_native_function(int argCount, Value *args) {
  ObjList* list = create_list();
  for (int i = 0; i < CLA.argc; i++) {
    push_back_to_list(list, make_string_sys(CLA.argv[i]));
  }
  return OBJ_VAL(list);
}

static Value get_argc_native_function(int argCount, Value *args) {
  return INT_VAL(CLA.argc);
}

void add_module_sys(const char* name, Value (*f)(int, Value*)) {
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

void sys_module_init() {
  add_module_sys("sys:get_argv", get_argv_native_function);
  add_module_sys("sys:get_argc", get_argc_native_function);
}

