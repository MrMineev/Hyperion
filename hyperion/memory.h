#ifndef memory_h
#define memory_h

#include <stdlib.h>

#include "object.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

int GROW_CAPACITY(int capacity);

#define GROW_ARRAY(type, pointer, old_size, new_size) \
    (type*)reallocate(pointer, sizeof(type) * (old_size), sizeof(type) * (new_size))

#define FREE_ARRAY(type, pointer, old_size) \
    reallocate(pointer, sizeof(type) * (old_size), 0)

void* reallocate(void* pointer, size_t old_size, size_t new_size);

void mark_object_memory(Obj *object);
void mark_memory_slot(Value value);
void take_out_garbage();

void free_objects();

#endif
