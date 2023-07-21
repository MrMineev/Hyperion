#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <time.h>

#include "time_module.h"
#include "../../HVM.h"
#include "../../value.h"

static Value clock_native_function(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

void time_module_init() {
  set_table(
      &hvm.globals,
      AS_STRING(
        OBJ_VAL(
          copy_string("clock", (int)strlen("clock"))
        )
      ),
      OBJ_VAL(
        create_native(clock_native_function)
      )
  );
}

