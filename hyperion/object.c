#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"
#include "table.h"
#include "HVM.h"

#define ALLOCATE_OBJ(type, objectType) (type*)allocate_object(sizeof(type), objectType)

static Obj* allocate_object(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;
  object->is_marked = false;

  object->next = hvm.objects;
  hvm.objects = object;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void*)object, size, type);
#endif

  return object;
}

ObjBoundMethod* create_bound_method(Value receiver, ObjClosure* method) {
  ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
  bound->receiver = receiver;
  bound->method = method;
  return bound;
}

ObjInstance* create_instance(ObjClass *_class) {
  ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
  instance->_class = _class;
  init_table(&instance->fields);
  return instance;
}

ObjClass* create_class(ObjString *name) {
  ObjClass *_class = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
  _class->name = name;
  init_table(&_class->methods);
  return _class;
}

ObjClosure* create_closure(ObjFunction* function) {
  ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvalueCount);
  for (int i = 0; i < function->upvalueCount; i++) {
    upvalues[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = function;
  closure->upvalues = upvalues;
  closure->upvalueCount = function->upvalueCount;
  return closure;
}

ObjFunction* create_function() {
  ObjFunction *obj_func = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  obj_func->arity = 0;
  obj_func->upvalueCount = 0;
  obj_func->name = NULL;
  create_chunk(&obj_func->chunk);
  return obj_func;
}

ObjUpvalue* create_upvalue(Value* slot) {
  ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  upvalue->closed = NIL_VAL;
  upvalue->location = slot;
  upvalue->next = NULL;
  return upvalue;
}

ObjNative* create_native(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

static ObjString* allocate_string(char* chars, int size, uint32_t hash) {
  ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  string->size = size;
  string->chars = chars;
  string->hash = hash;

  push(OBJ_VAL(string));
  set_table(&hvm.strings, string, NIL_VAL);
  pop();

  return string;
}

static uint32_t hash_string(const char *key, int size) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < size; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

ObjString* take_string(char *chars, int size) {
  uint32_t hash = hash_string(chars, size);
  ObjString* interned = table_find_string(&hvm.strings, chars, size, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, size + 1);
    return interned;
  }
  return allocate_string(chars, size, hash);
}

ObjString* copy_string(const char* chars, int size) {
  uint32_t hash = hash_string(chars, size);

  ObjString* interned = table_find_string(&hvm.strings, chars, size, hash);
  if (interned != NULL) return interned;

  char* heap_chars = ALLOCATE(char, size + 1);
  memcpy(heap_chars, chars, size);
  heap_chars[size] = '\0';
  return allocate_string(heap_chars, size, hash);
}

void print_function(ObjFunction *func) {
  if (func->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", func->name->chars);
}

static void print_list(ObjList* list) {
  printf("[");
  for (int i = 0; i < list->count - 1; i++) {
    print_value(list->items[i]);
    printf(", ");
  }
  if (list->count != 0) {
    print_value(list->items[list->count - 1]);
  }
  printf("]");
}

void print_object(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_BOUND_METHOD:
      print_function(AS_BOUND_METHOD(value)->method->function);
      break;
    case OBJ_INSTANCE:
      printf("%s instance", AS_INSTANCE(value)->_class->name->chars);
      break;
    case OBJ_CLASS:
      printf("%s", AS_CLASS(value)->name->chars);
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_FUNCTION:
      print_function(AS_FUNCTION(value));
      break;
    case OBJ_CLOSURE:
      print_function(AS_CLOSURE(value)->function);
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_UPVALUE:
      printf("upvalue");
      break;
    case OBJ_LIST:
      print_list(AS_LIST(value));
      break;
  }
}

ObjList* create_list() {
    ObjList* list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return list;
}

void push_back_to_list(ObjList* list, Value value) {
  if (list->capacity < list->count + 1) {
    int oldCapacity = list->capacity;
    list->capacity = GROW_CAPACITY(oldCapacity);
    list->items = GROW_ARRAY(Value, list->items, oldCapacity, list->capacity);
  }
  list->items[list->count] = value;
  list->count++;
  return;
}

void store_to_list(ObjList* list, int index, Value value) {
    list->items[index] = value;
}

Value index_from_list(ObjList* list, int index) {
    return list->items[index];
}

void delete_from_list(ObjList* list, int index) {
  for (int i = index; i < list->count - 1; i++) {
    list->items[i] = list->items[i + 1];
  }
  list->items[list->count - 1] = NIL_VAL;
  list->count--;
}

bool is_valid_list_index(ObjList* list, int index) {
  if (index < 0 || index > list->count - 1) {
    return false;
  }
  return true;
}

