#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "list.h"
#include "../../HVM.h"
#include "../../value.h"
#include "../../object.h"

static Value push_back_native_function(int argCount, Value* args) {
  if (!IS_LIST(args[0])) {
    return NIL_VAL;
  }

  ObjList* list = AS_LIST(*args);
  Value item = args[1];
  push_back_to_list(list, item);

  return NIL_VAL;
}

static Value erase_native_function(int argCount, Value *args) {
  if (!IS_INT(args[1])) {
    return NIL_VAL;
  }

  ObjList* list = AS_LIST(args[0]);
  int index = AS_INT(args[1]);

  if (!is_valid_list_index(list, index)) {
      return NIL_VAL;
  }

  delete_from_list(list, index);
  return NIL_VAL;
}

static Value init_native_function(int argCount, Value *args) {
  ObjList *list = create_list();
  for (int i = 0; i < AS_INT(args[0]); i++) {
    push_back_to_list(list, args[1]);
  }
  return OBJ_VAL(list);
}

static Value len_native_function(int argCount, Value *args) {
  return INT_VAL(
      AS_LIST(args[0])->count
  );
}

void add_module_list(const char* name, Value (*f)(int, Value*)) {
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

void list_module_init() {
  add_module_list("list:push_back", push_back_native_function);
  add_module_list("list:erase", erase_native_function);
  add_module_list("list:init", init_native_function);
  add_module_list("list:len", len_native_function);
}

