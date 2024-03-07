#ifndef APEX_HASH_H
#define APEX_HASH_H

#include "value.h" 

typedef struct ApexHash_entry {
    char *key;
    ApexValue *value; 
    struct ApexHash_entry *next;
    size_t index;
} ApexHash_entry;

struct ApexHash_table {
    ApexHash_entry **entries;
    size_t size;
};

#ifndef APEX_HASH_TABLE_DEFINED
#define APEX_HASH_TABLE_DEFINED
    typedef struct ApexHash_table ApexHash_table;
#endif

typedef enum {
    APEX_HASH_NODE_VALUE,
    APEX_HASH_NODE_TABLE
} ApexHash_node_type;

typedef unsigned int HashKey;

extern ApexHash_table *ApexHash_create_table(int);
extern void ApexHash_table_insert_str(ApexHash_table *, const char *, const char *);
extern void ApexHash_table_insert_int(ApexHash_table *, const char *, int);
extern void ApexHash_table_insert_func(ApexHash_table *, const char *, ApexValue_function);
ApexValue *ApexHash_table_get_value(ApexHash_table *, const char *);
ApexHash_entry *ApexHash_table_get_entry(ApexHash_table *, const char *);
extern void ApexHash_free_table(ApexHash_table *);

#endif