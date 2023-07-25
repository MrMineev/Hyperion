#include <stdlib.h>

#include "HVM.h"
#include "compiler.h"
#include "memory.h"

#ifdef DEBUG_LOG_GC

#include <stdio.h>
#include "debug.h"

#endif

int GROW_CAPACITY(int capacity) {
  return (capacity < 8) ? 8 : capacity * 2;
}

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
  hvm.bytes_alloc += new_size - old_size;

  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    take_out_garbage();
#endif
  }

  if (hvm.bytes_alloc > hvm.next_gc_limit) {
    take_out_garbage();
  }

  if (new_size == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, new_size);
  if (result == NULL) exit(1);
  return result;
}

void mark_object_memory(Obj *object) {
  if (object == NULL) return;
  if (object->is_marked) return;

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  print_value(OBJ_VAL(object));
  printf("\n");
#endif

  object->is_marked = true;

  if (hvm.gray_capacity < hvm.gray_cnt + 1) {
    hvm.gray_capacity = GROW_CAPACITY(hvm.gray_capacity);
    hvm.gray_stack = (Obj**)realloc(hvm.gray_stack, sizeof(Obj*) * hvm.gray_capacity);
  }
  hvm.gray_stack[hvm.gray_cnt++] = object;

  if (hvm.gray_stack == NULL) {
    exit(1);
  }
}

void mark_memory_slot(Value value) {
  if (IS_OBJ(value)) mark_object_memory(AS_OBJ(value));
}

static void mark_array_memory(ValueArray* array) {
  for (int i = 0; i < array->size; i++) {
    mark_memory_slot(array->values[i]);
  }
}

static void visit_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  print_value(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_LIST: {
      ObjList* list = (ObjList*)object;
      for (int i = 0; i < list->count; i++) {
        mark_memory_slot(list->items[i]);
      }
      break;
    }
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod* bound = (ObjBoundMethod*)object;
      mark_memory_slot(bound->receiver);
      mark_object_memory((Obj*)bound->method);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      mark_object_memory((Obj*)instance->_class);
      mark_table(&instance->fields);
      break;
    }
    case OBJ_CLASS: {
      ObjClass *_class = (ObjClass*)object;
      mark_object_memory((Obj*)_class->name);
      mark_table(&_class->methods);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      mark_object_memory((Obj*)closure->function);
      for (int i = 0; i < closure->upvalueCount; i++) {
        mark_object_memory((Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      mark_object_memory((Obj*)function->name);
      mark_array_memory(&function->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      mark_memory_slot(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

static void free_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

  switch (object->type) {
    case OBJ_LIST: {
      ObjList* list = (ObjList*)object;
      FREE_ARRAY(Value*, list->items, list->count);
      FREE(ObjList, object);
      break;
    }
    case OBJ_BOUND_METHOD:
      FREE(ObjBoundMethod, object);
      break;
    case OBJ_INSTANCE: {
      ObjInstance* instance = (ObjInstance*)object;
      free_table(&instance->fields);
      FREE(ObjInstance, object);
      break;
    }
    case OBJ_CLASS: {
      ObjClass* _class = (ObjClass*)object;
      free_table(&_class->methods);
      FREE(ObjClass, object);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalueCount);
      FREE(ObjClosure, object);
      break;
    }
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
    case OBJ_NATIVE:
      FREE(ObjNative, object);
      break;
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
  }
}


static void goodbye_old_friends() {
  Obj* previous = NULL;
  Obj* object = hvm.objects;
  while (object != NULL) {
    if (object->is_marked) {
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        object->is_marked = false;
        previous->next = object;
      } else {
        hvm.objects = object;
      }

      free_object(unreached);
    }
  }
}

static void mark_roots() {
  for (Value* slot = hvm.stack; slot < hvm.top; slot++) {
    mark_memory_slot(*slot);
  }

  for (int i = 0; i < hvm.frameCount; i++) {
    mark_object_memory((Obj*)hvm.frames[i].closure);
  }

  for (ObjUpvalue* upvalue = hvm.openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
    mark_object_memory((Obj*)upvalue);
  }

  mark_table(&hvm.globals);

  mark_compiler_roots();

  mark_object_memory((Obj*)hvm.initString);
}

static void visit_nodes() {
  while (hvm.gray_cnt > 0) {
    Obj* object = hvm.gray_stack[--hvm.gray_cnt];
    visit_object(object);
  }
}

void take_out_garbage() {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  size_t before = hvm.bytes_alloc;
#endif

  mark_roots();
  visit_nodes();
  goodbye_white_table_friends(&hvm.strings);
  goodbye_old_friends();

  hvm.next_gc_limit = hvm.bytes_alloc * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - hvm.bytes_alloc, before, hvm.bytes_alloc,
         hvm.next_gc_limit);
#endif
}

void free_objects() {
  Obj* object = hvm.objects;
  while (object != NULL) {
    Obj* next = object->next;
    free_object(object);
    object = next;
  }

  free(hvm.gray_stack);
}



