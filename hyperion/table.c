#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table *table) {
  table->size = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void free_table(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}

static Entry* find_entry(Entry *entries, int capacity, ObjString *key) {
  // the core for the hash table
  // for now using linear search
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;
  while (true) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }
    index = (index + 1) % capacity;
  }
}

bool table_get(Table* table, ObjString* key, Value* value) {
  if (table->size == 0) return false;
  Entry* entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;
  *value = entry->value;
  return true;
}

static void adjust_capacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }
  table->size = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    // this is why the hash table speed isn't always growing
    // when the capacity is in need of growth we have to reinstall all the entries
    Entry* dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->size++;
  }
  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool set_table(Table *table, ObjString *key, Value value) {
  if (table->size + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }
  Entry *entry = find_entry(table->entries, table->capacity, key);
  bool is_new = (entry->key == NULL);
  if (is_new && IS_NIL(entry->value)) table->size++;

  entry->key = key;

  return is_new;
}

bool table_delete(Table* table, ObjString* key) {
  if (table->size == 0) return false;
  Entry* entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void table_add_all(Table *from, Table *to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry *entry = &from->entries[i];
    if (entry->key != NULL) {
      set_table(to, entry->key, entry->value);
    }
  }
}

ObjString* table_find_string(Table* table, const char* chars, int size, uint32_t hash) {
  if (table->size == 0) return NULL;
  uint32_t index = hash % table->capacity;
  while (true) {
    Entry* entry = &table->entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->size == size &&
               entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, size) == 0) {
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}

