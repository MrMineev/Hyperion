#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <time.h>

#include "time.h"
#include "../../HVM.h"
#include "../../value.h"

static Value clock_native_function(int argCount, Value* args) {
  return NUMBER_VAL((double)clock() / 1000000);
}

void time_module_init() {
  set_table(
      &hvm.globals,
      AS_STRING(
        OBJ_VAL(
          copy_string("time:clock", (int)strlen("time:clock"))
        )
      ),
      OBJ_VAL(
        create_native(clock_native_function)
      )
  );
}

