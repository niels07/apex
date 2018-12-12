#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>

#include <apex/value.h>
#include <apex/api.h>
#include "malloc.h"
#include "hash.h"
#include "error.h"

#define MODULE_TABLE_SIZE 256

typedef struct {
    char *str;
    size_t len;   
} NodeKey;

typedef union {
    ApexValue value;
    ApexHashTable *table;
} NodeData;

typedef struct node {
    ApexNodeType type;
    NodeKey key;
    NodeData data;
    struct node *next;
} Node;

struct ApexHashTable {
    size_t size;
    size_t count;
    Node **nodes;
};

#define HAVE_KEY(node, key) \
    (strncmp((node)->key.str, key, node->key.len) == 0)

HashKey apex_do_hash(const char *str) {
    HashKey h = 0;

    while (*str) {
        h = ((h << 5) + h) + *str++;
    }
    return h;
}

static Node *get_node(ApexHashTable *table, const char *key) {
    HashKey h;
    Node *node;

    if (!table) {
        return NULL;
    }

    h = apex_do_hash(key) % table->size;
    node = table->nodes[h];

    while (node != NULL) {
        if (HAVE_KEY(node, key)) {
            return node;
        } else {
            node = node->next;
        }
    }
    return NULL;
}

static void set_node_key(Node *node, const char *key) {
    node->key.len = strlen(key);
    node->key.str = apex_malloc(node->key.len + 1);
    strncpy(node->key.str, key, node->key.len);
    node->key.str[node->key.len] = '\0';
}

static Node *node_new(const char *key, const NodeData *data) {
    Node *node = APEX_NEW(Node);

    set_node_key(node, key);
    node->data = *data;
    node->next = NULL;
    return node;
}

static Node *table_new_entry(
    ApexHashTable *table,
    const char *key,
    const NodeData *data) {

    HashKey h = apex_do_hash(key) % table->size;
    Node *node = table->nodes[h];

    while (node && node->next) {
        if (HAVE_KEY(node, key)) {
            node->data = *data;
            return node;
        }
        node = node->next;
    }
    
    node = node_new(key, data);
    node->next = table->nodes[h];

    table->count++;
    table->nodes[h] = node;
    return node;
}

static void free_node(Node *node) {
    apex_free(node->key.str);
    apex_free(node);
}

ApexHashTable *apex_hash_table_new(size_t size) {
    ApexHashTable *table = APEX_NEW(ApexHashTable);
    
    table->nodes = apex_calloc(size, sizeof(Node));
    table->size = size;
    table->count = 0;
    return table;
}

ApexHashTable *resize_table(ApexHashTable *table, size_t size) {
    size_t i;
    ApexHashTable *ntable = apex_hash_table_new(size);

    for (i = 0; i < table->size; i++) {
        Node *node = table->nodes[i];
        HashKey h;
            
        if (node) {
            HashKey h = apex_do_hash(node->key.str) % size;
            ntable->nodes[h] = node;
        }
    }
    apex_free(table->nodes);
    apex_free(table);
    return ntable;
}

void apex_hash_table_set_table(
    ApexHashTable *table,
    const char *key,
    ApexHashTable *child_table) {
    
    NodeData data;
    Node *node;
    
    data.table = child_table;
    node = table_new_entry(table, key, &data);
    node->type = APEX_NODE_TABLE;
}

void apex_hash_table_set_value(
    ApexHashTable *table,
    const char *key,
    const ApexValue *value) {

    NodeData data;
    Node *node;
    
    data.value = *value;
    node = table_new_entry(table, key, &data);
    node->type = APEX_NODE_VALUE;
}

void apex_hash_table_remove(
    ApexHashTable *table,
    const char *key) {

    HashKey h = apex_do_hash(key) % table->size;
    Node *node = table->nodes[h];

    while (node) {
        Node *prev;

        if (!HAVE_KEY(node, key)) {
            prev = node;
            node = node->next;
            continue;
        }
        if (prev) {
            prev->next = node->next;
        } else {
            table->nodes[h] = node->next;
        }
        free_node(node);
        break;
    }
}

void apex_hash_table_destroy(ApexHashTable *table) {
    size_t i;

    for (i = 0; i < table->size; i++) {
        Node *node = table->nodes[i];
        while (node) {
            Node *prev = node;
            node = node->next;
            free_node(prev);
        }
    }
    apex_free(table->nodes);
    apex_free(table);
}

ApexValue *apex_hash_table_get_value(
    ApexHashTable *table,
    const char *key) {

    Node *node = get_node(table, key);
    return node && node->type == APEX_NODE_VALUE
        ? &node->data.value
        : NULL;
}

ApexHashTable *apex_hash_table_get_table(
    ApexHashTable *table,
    const char *key) {

    Node *node = get_node(table, key);
    return node && node->type == APEX_NODE_TABLE 
        ? node->data.table 
        : NULL;
}

void apex_hash_table_import(ApexHashTable *table, const char *module_name) {
    char path[256];
    void *handle;
    struct ApexApiExports *exports;
    struct ApexApiMethods *methods;
    ApexHashTable *module;
    ApexValue value;
    size_t i;       
    extern void apex_api_init_methods(struct ApexApiMethods *);

    sprintf(path, "modules/lib%s.so", module_name);
     
    handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        apex_error_io("unable to load %s", path);
    }
    exports = dlsym(handle, "apex_api_exports");
    methods = dlsym(handle, "apex_api_methods");
    apex_api_init_methods(methods);

    module = apex_hash_table_get_table(table, module_name);

    if (!module) {
        module = apex_hash_table_new(MODULE_TABLE_SIZE);
        apex_hash_table_set_table(table, module_name, module);
    }

    for (i = 0; exports[i].name; i++) {
        switch (exports[i].type) {
        case APEX_TYPE_INT:       
            apex_value_make_int(&value, exports[i].intval);    
            break;

        case APEX_TYPE_FUNC:       
            apex_value_make_func(&value, exports[i].func);          
            break;        
        }
        apex_hash_table_set_value(table, exports[i].name, &value);        
    }
}


