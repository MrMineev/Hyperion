#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "os.h"
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

static Value getcwd_native_function(int argCount, Value *args) {
  char cwd[256];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    return make_string_os(cwd);
  } else {
    return NIL_VAL;
  }
}

void add_module_os(const char* name, Value (*f)(int, Value*)) {
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

void os_module_init() {
  add_module_os("os:getcwd", getcwd_native_function);
}

