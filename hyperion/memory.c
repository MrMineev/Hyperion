#include <stdlib.h>

#include "HVM.h"
#include "object.h"

int GROW_CAPACITY(int capacity) {
  return (capacity < 8) ? 8 : capacity * 2;
}

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, new_size);
  if (result == NULL) exit(1);
  return result;
}

static void free_object(Obj* object) {
  switch (object->type) {
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->size + 1);
      FREE(ObjString, object);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction *function = (ObjFunction*)object;
      free_chunk(&function->chunk);
      FREE(ObjFunction, object);
      break;
    }
  }
}

void free_objects() {
  Obj* object = hvm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    free_object(object);
    object = next;
  }
}

