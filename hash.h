#ifndef APEX_HASH_H
#define APEX_HASH_H

#include "value.h"

typedef struct ApexHashTable ApexHashTable;

typedef enum {
    APEX_NODE_VALUE,
    APEX_NODE_TABLE
} ApexNodeType;

extern void apex_hash_table_set_table(ApexHashTable *, const char *, ApexHashTable *);
extern void apex_hash_table_set_value(ApexHashTable *, const char *, const ApexValue *);
extern ApexHashTable *apex_hash_table_new(size_t);

#endif
