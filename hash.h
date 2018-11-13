#ifndef APEX_HASH_H
#define APEX_HASH_H

#include "value.h"

typedef struct ApexHashTable ApexHashTable;

typedef enum {
    APEX_NODE_VALUE,
    APEX_NODE_TABLE
} ApexNodeType;

typedef unsigned int HashKey;

extern HashKey apex_do_hash(const char *);
extern void apex_hash_table_set_table(ApexHashTable *, const char *, ApexHashTable *);
extern void apex_hash_table_set_value(ApexHashTable *, const char *, const ApexValue *);
extern ApexHashTable *apex_hash_table_new(size_t);
extern void apex_hash_table_import(ApexHashTable *, const char *);

extern ApexValue *apex_hash_table_get_value(ApexHashTable *, const char *);

#endif
