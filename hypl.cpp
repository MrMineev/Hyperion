#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hyperion/chunk.h"
#include "hyperion/debug.h"
#include "hyperion/HVM.h"

static void repl() {
  char line[1024];
  while (true) {
    printf("[HYPL]-> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

static char* get_source_content(const char* path) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "[ERROR] Could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "[ERROR] Not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "[ERROR] Could not read file \"%s\".\n", path);
    exit(74);
  }

  buffer[bytesRead] = '\0';

  fclose(file);
  return buffer;
}

static void run_file(const char* path) {
  char* source = get_source_content(path);
  InterReport result = interpret(source);
  free(source); 

  if (result == INTER_COMPILE_ERROR) exit(65);
  if (result == INTER_RUNTIME_ERROR) exit(70);
}

int main(int argc, char* argv[]) {
  init_hvm();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    fprintf(stderr, "[ERROR] Usage: hypl [path]\n");
    exit(64);
  }

  free_hvm();
  return 0;
}

