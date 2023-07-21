#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "file_io.h"
#include "../../HVM.h"
#include "../../value.h"
#include "../../object.h"

static Value make_string_file_io(char* str) {
  return 
    OBJ_VAL(
      copy_string(
        str,
        (int)strlen(str)
      )
    );
}

static Value file_io_read_native_function(int argCount, Value* args) {
  char* file_path = AS_STRING(args[0])->chars;

  FILE* file = fopen(file_path, "r");
  if (file == NULL) {
    fprintf(stderr, "Error opening file: %s\n", file_path);
    return NIL_VAL;
  }
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* content = (char*)malloc(file_size + 1);
  if (content == NULL) {
    fclose(file);
    fprintf(stderr, "Memory allocation failed.\n");
    return NIL_VAL;
  }
  size_t bytes_read = fread(content, 1, file_size, file);
  if (bytes_read != file_size) {
    fclose(file);
    free(content);
    fprintf(stderr, "Error reading file: %s\n", file_path);
    return NIL_VAL;
  }
  content[file_size] = '\0';
  fclose(file);

  return make_string_file_io(content);
}

static Value file_io_output_native_function(int argCount, Value *args) {
  char* file_path = AS_STRING(args[0])->chars;

  FILE* file = fopen(file_path, "w");
  if (file == NULL) {
      fprintf(stderr, "Error opening file: %s\n", file_path);
      return NIL_VAL;
  }

  // Write the content to the file
  fputs(AS_STRING(args[1])->chars, file);

  fclose(file);

  return NIL_VAL;
}

void add_module_file_io(const char* name, Value (*f)(int, Value*)) {
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

void file_io_module_init() {
  add_module_file_io("file_io:read", file_io_read_native_function);
  add_module_file_io("file_io:write", file_io_output_native_function);
}

