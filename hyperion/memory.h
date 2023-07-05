#ifndef memory_h
#define memory_h

#include <cstdlib>

int GROW_CAPACITY(int capacity) {
  return (capacity < 8) ? 8 : capacity * 2;
}

#define GROW_ARRAY(type, pointer, old_size, new_size) \
    (type*)reallocate(pointer, sizeof(type) * (old_size), sizeof(type) * (new_size))

#define FREE_ARRAY(type, pointer, old_size) \
    reallocate(pointer, sizeof(type) * (old_size), 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, new_size);
  if (result == NULL) exit(1);
  return result;
}


#endif
