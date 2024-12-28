#include <string.h>
#include <stdlib.h>
#include "value.h"
#include "vm.h"
#include "mem.h"
#include "string.h"
#include "error.h"

const char *get_value_type_str(ValueType type) {
    switch (type) {
    case APEX_VAL_INT:
        return "int";
    case APEX_VAL_FLT:
        return "float";
    case APEX_VAL_STR:
        return "string";
    case APEX_VAL_BOOL:
        return "bool";
    default:
        break;
    }
    return "null";
}

#define ARR_INIT_SIZE 16
#define ARR_LOAD_FACTOR 0.75

static unsigned int hash_string(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash << 5) + hash + (unsigned char)(*str);
        str++;
    }
    return hash;
}

static unsigned int get_array_index(const ApexValue key) {
    switch (key.type) {
    case APEX_VAL_INT:
        return key.intval;
    case APEX_VAL_STR:
        return hash_string(key.strval);
    case APEX_VAL_BOOL:
        return (unsigned int)key.boolval;
    case APEX_VAL_FLT: {
        union { float f; unsigned int i; } flt_to_int;
        flt_to_int.f = key.fltval;
        return flt_to_int.i;
    }
    default:
        return 0;
    }
}

static Bool value_equals(const ApexValue a, const ApexValue b) {
    if (a.type != b.type) {
        return 0;
    }
    switch (a.type) {
    case APEX_VAL_INT:
        return a.intval == b.intval;
    case APEX_VAL_STR:
        return a.strval == b.strval;
    case APEX_VAL_BOOL:
        return a.boolval == b.boolval;
    case APEX_VAL_FLT:
        return a.fltval == b.fltval;
    case APEX_VAL_NULL:
        return 1;
    default:
        return 0;
    }
}

static void retain_value(ApexValue *value) {
    if (value->type == APEX_VAL_ARR) {
        value->refcount++;
    }
}

static void release_value(ApexValue *value) {
    if (value->type == APEX_VAL_ARR) {
        value->refcount--;
        if (value->refcount == 0) {
            free_value(*value);
        }
    }
}

Array *apexVal_newarray(void) {
    Array *array = mem_alloc(sizeof(Array));
    array->entries = mem_calloc(sizeof(Array), ARR_INIT_SIZE);
    array->size = ARR_INIT_SIZE;
    array->n = 0;    
    return array;
}

void apexVal_freearray(Array *array) {
    int i;
    for (i = 0; i < array->size; i++) {
        ArrayEntry *entry = array->entries[i];
        while (entry) {
            ArrayEntry *next = entry->next;
            release_value(&entry->key);
            release_value(&entry->value);
            free(entry);
            entry = next;
        }
    }
    free(array->entries);
    free(array);
}

static void array_resize(Array *array) {
    int new_size = array->size * 2;
    ArrayEntry **new_entries = mem_calloc(new_size, sizeof(ArrayEntry *));
    int i;

    for (i = 0; i < array->size; i++) {
        ArrayEntry *entry = array->entries[i];
        while (entry) {
            unsigned int index = get_array_index(entry->key) % new_size;
            ArrayEntry *next = entry->next;
            entry->next = new_entries[index];
            new_entries[index] = entry;
            entry = next;
        }
    }

    free(array->entries);
    array->entries = new_entries;
    array->size = new_size;
}

void apexVal_arrayset(Array *array, ApexValue key, ApexValue value) {
    unsigned int index = get_array_index(key) % array->size;
    ArrayEntry *entry = array->entries[index];

    while (entry) {
        if (value_equals(entry->key, key)) {
            release_value(&entry->value);
            retain_value(&value);
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    entry = mem_alloc(sizeof(ArrayEntry));
    retain_value(&key); 
    retain_value(&value);
    entry->key = key;
    entry->value = value;
    entry->next = array->entries[index];
    array->entries[index] = entry;
    array->n++;

    if ((float)array->n / array->size > ARR_LOAD_FACTOR) {
        array_resize(array);
    }
}

Bool apexVal_arrayget(ApexValue *value, Array *array, const ApexValue key) {
    unsigned int index = get_array_index(key) % array->size;
    ArrayEntry *entry = array->entries[index];

    while (entry) {
        if (value_equals(entry->key, key)) {
            *value = entry->value;
            return TRUE;
        }
        entry = entry->next;
    }

    return FALSE;
}

void apexVal_arraydel(Array *array, const ApexValue key) {
    unsigned int index = get_array_index(key) % array->size;
    ArrayEntry *prev = NULL;
    ArrayEntry *entry = array->entries[index];

    while (entry) {
        if (value_equals(entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                array->entries[index] = entry->next;
            }
            release_value(&entry->key);
            release_value(&entry->value);
            free(entry);
            array->n--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

ApexValue apexVal_makeint(int value) {
    ApexValue v;
    v.type = APEX_VAL_INT;
    v.intval = value;
    return v;
}

ApexValue apexVal_makeflt(float value) {
    ApexValue v;
    v.type = APEX_VAL_FLT;
    v.fltval = value;
    return v;
}

/**
 * Creates an ApexValue with the given string value.
 *
 * @param value The string value to associate with the ApexValue.
 *
 * @return An ApexValue with the given string value.
 */
ApexValue apexVal_makestr(char *value) {
    ApexValue v;
    v.type = APEX_VAL_STR;
    v.strval = value;
    return v;
}

/**
 * Creates an ApexValue with the given boolean value.
 *
 * @param value The boolean value to associate with the ApexValue.
 *
 * @return An ApexValue with the given boolean value.
 */
ApexValue apexVal_makebool(Bool value) {
    ApexValue v;
    v.type = APEX_VAL_BOOL;
    v.boolval = value;
    v.refcount = 1;
    return v;
}

ApexValue apexVal_makefn(Fn *fn) {
    ApexValue v;
    v.type = APEX_VAL_FN;
    v.fnval = fn;
    v.refcount = 1;
    return v;
}

ApexValue apexVal_makearr(Array *arr) {
    ApexValue v;
    v.type = APEX_VAL_ARR;
    v.arrval = arr;
    v.refcount = 1;
    return v;
}

ApexValue apexVal_makenull(void) {
    ApexValue v;
    v.type = APEX_VAL_NULL;
    v.refcount = 1;
    return v;
}

void free_value(ApexValue value) {
    if (value.type == APEX_VAL_FN) {
        free(value.fnval->params);
        free(value.fnval);        
    } else if (value.type == APEX_VAL_ARR) {
        apexVal_freearray(value.arrval);
    }
}