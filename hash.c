#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "malloc.h"
#include "types.h"

ApexHash_table *ApexHash_create_table(int size) {
    ApexHash_table *ht = APEX_NEW(ApexHash_table) ;
    ht->size = size;
    ht->entries = apex_calloc(size, sizeof(ApexHash_entry *));
    return ht;
}

static Uint hash(const char *key, int size) {
    Uint hashval = 0;
    for (; *key != '\0'; key++) {
        hashval = *key + 31 * hashval;
    }
    return hashval % size;
}

void ApexHash_table_insert_int(ApexHash_table *ht, const char *key, int value) {
    Uint index = hash(key, ht->size);
    ApexHash_entry *entry = ht->entries[index];
    while (entry != NULL && strcmp(entry->key, key) != 0) {
        entry = entry->next;
    }
    if (entry == NULL) {
        entry = APEX_NEW(ApexHash_entry);
        entry->key = dupstr(key);
        entry->value->data.int_val = value;
        entry->next = ht->entries[index];
        ht->entries[index] = entry;
    } else {
        entry->value->data.int_val = value;
    }
    entry->index = index;
}

void ApexHash_table_insert_str(ApexHash_table *ht, const char *key, const char *value) {
    Uint index = hash(key, ht->size);
    ApexHash_entry *entry = ht->entries[index];
    while (entry != NULL && strcmp(entry->key, key) != 0) {
        entry = entry->next;
    }
    if (entry == NULL) {
        entry = APEX_NEW(ApexHash_entry);
        entry->key = dupstr(key);
        entry->value->data.str_val = dupstr(value);
        entry->next = ht->entries[index];
        ht->entries[index] = entry;
    } else {
        free(entry->value->data.str_val); 
        entry->value->data.str_val = dupstr(value);
    }
    entry->index = index;
}

void ApexHash_table_insert_func(ApexHash_table *ht, const char *key, ApexValue_function func) {
    Uint index = hash(key, ht->size);
    ApexHash_entry *entry = ht->entries[index];
    while (entry != NULL && strcmp(entry->key, key) != 0) {
        entry = entry->next;
    }
    if (entry == NULL) {
        entry = APEX_NEW(ApexHash_entry);
        entry->key = dupstr(key);
        entry->value->data.func_val = func; 
        entry->next = ht->entries[index];
        ht->entries[index] = entry;
    } else {
        entry->value->data.func_val = func;
    }
    entry->index = index;
}

ApexHash_entry *ApexHash_table_get_entry(ApexHash_table *ht, const char *key) {
    Uint index = hash(key, ht->size);
    ApexHash_entry *entry = ht->entries[index];
    
    while (entry != NULL && strcmp(entry->key, key) != 0) {
        entry = entry->next;
    }
    return (entry != NULL) ? entry: NULL; 
}

ApexValue *ApexHash_table_get_value(ApexHash_table *ht, const char *key) {
    Uint index = hash(key, ht->size);
    ApexHash_entry *entry = ht->entries[index];
    
    while (entry != NULL && strcmp(entry->key, key) != 0) {
        entry = entry->next;
    }
    return (entry != NULL) ? entry->value : NULL; 
}

void ApexHash_free_table(ApexHash_table *ht) {
    int i;
    for (i = 0; i < ht->size; i++) {
        ApexHash_entry *entry = ht->entries[i];
        while (entry != NULL) {
            ApexHash_entry *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(ht->entries);
    free(ht);
}