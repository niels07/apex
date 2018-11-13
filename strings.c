#include <string.h>
#include "malloc.h"

#include "hash.h"
#include "strings.h"

#define STR_TABLE_INIT_SIZE 256

struct ApexString {
    size_t len;
    char *value;
    ApexString *next;
};

struct ApexStringTable {
    size_t size;
    size_t n;
    ApexString **entries;
};

static void string_table_resize(ApexStringTable *tbl) {
    size_t size = tbl->size * 2;
    ApexString **entries = apex_calloc(size, sizeof(ApexString));
    size_t i;

    for (i = 0; i < tbl->size; i++) {
        ApexString *str = tbl->entries[i];
        size_t h;
        
        if (str) {
            h = apex_do_hash(str->value) % size;
            entries[h] = str;
        }
    }
    apex_free(tbl->entries);
    tbl->entries = entries;
}

static void checkTableSize(ApexStringTable *tbl) {
    if (tbl->n == tbl->size) {
        string_table_resize(tbl);
    }
}

static ApexString *newstr(const char *value) {
    ApexString *str = APEX_NEW(ApexString);

    str->len = strlen(value);
    str->value = apex_malloc(str->len + 1);
    strncpy (str->value, value, str->len);
    str->value[str->len] = '\0';
    return str;
}

ApexStringTable *apex_string_table_new(void) {
    ApexStringTable *tbl = APEX_NEW(ApexStringTable);

    tbl->n = 0;
    tbl->size = STR_TABLE_INIT_SIZE;
    tbl->entries = apex_calloc(tbl->size, sizeof(ApexString *));
    return tbl;
}

ApexString *apex_string_new(ApexStringTable *tbl, const char *value) {
    ApexString *str;
    size_t h = apex_do_hash(value) % tbl->size;
    size_t l = strlen(value);
    
    str = tbl->entries[h];
    checkTableSize(tbl);

    while (str) {
        if (str->len == l && memcmp(str->value, value, l) == 0) {
            return str;
        }
        str = str->next;
    }

    str = newstr(value);
    str->next = tbl->entries[h];
    tbl->entries[h] = str;
    tbl->n++;
    return str;
}

char *apex_string_to_cstr(ApexString *str) {
    return str->value;
}

void apex_string_table_destroy(ApexStringTable *tbl) {
    size_t i = 0;
    ApexString *str, *next;
          
    while (i < tbl->size) {
        str = tbl->entries[i++];
        while (str) {
            next = str->next;
            apex_free(str->value);
            apex_free(str);
            str = next;
        }
    }
    apex_free(tbl->entries);
    apex_free(tbl);
}
