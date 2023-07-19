#ifndef table_h
#define table_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int size;
  int capacity;
  Entry *entries;
} Table;

void init_table(Table *table);
void free_table(Table *table);
bool set_table(Table *table, ObjString *key, Value value);
bool table_delete(Table* table, ObjString* key);
bool table_get(Table* table, ObjString* key, Value* value);
void table_add_all(Table* from, Table* to);
ObjString* table_find_string(Table* table, const char* chars, int size, uint32_t hash);

void goodbye_white_table_friends(Table *table);
void mark_table(Table *table);

#endif
