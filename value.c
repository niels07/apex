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

/**
 * Computes a hash value for an ApexValue suitable for use as an index in an array.
 *
 * This function takes an ApexValue and returns a hash value based on its type and value.
 * The hash values are computed as follows:
 * - Integers are returned as-is.
 * - Strings are hashed using a simple string hashing algorithm.
 * - Booleans are converted to an unsigned integer.
 * - Floats are converted to an unsigned integer using a union.
 * - Null values are given a hash value of 0.
 *
 * The hash value is then used as an index in an array.
 *
 * @param key The ApexValue to compute a hash value for.
 */
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

/**
 * Compares two ApexValue objects for equality.
 *
 * This function checks if two ApexValue objects are equal by comparing their
 * types and values. The comparison is performed based on the type of the values:
 * - For integers, their integer values are compared.
 * - For strings, their string pointers are compared.
 * - For booleans, their boolean values are compared.
 * - For floats, their float values are compared.
 * - For null values, they are considered equal.
 * 
 * If the types of the two values do not match, the function returns FALSE.
 *
 * @param a The first ApexValue to compare.
 * @param b The second ApexValue to compare.
 * 
 * @return TRUE if the two ApexValue objects are equal, otherwise FALSE.
 */
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

/**
 * Increments the reference count of a value if it is an array.
 *
 * This function increments the reference count of the given value
 * if it is of type array. This should be called when the value is
 * assigned to a variable, passed as an argument to a function, or
 * returned from a function. The reference count ensures that the
 * memory allocated for the array and its contents is not freed until
 * the value is no longer referenced.
 *
 * @param value A pointer to the ApexValue whose reference is to be
 *              incremented.
 */
static void retain_value(ApexValue *value) {
    if (value->type == APEX_VAL_ARR) {
        value->refcount++;
    }
}

/**
 * Releases a reference to an ApexValue if it is an array.
 *
 * This function decrements the reference count of the given value
 * if it is of type array. If the reference count reaches zero, 
 * the memory allocated for the array and its contents is freed.
 *
 * @param value A pointer to the ApexValue whose reference is to be released.
 */
static void release_value(ApexValue *value) {
    if (value->type == APEX_VAL_ARR) {
        value->refcount--;
        if (value->refcount == 0) {
            free_value(*value);
        }
    }
}

/**
 * Creates a new array with the given size.
 *
 * This function allocates memory for the array and its entries, and
 * initializes the array with the given size. The array is not
 * initialized with any values.
 *
 * @return A pointer to the newly created array.
 */
Array *apexVal_newarray(void) {
    Array *array = mem_alloc(sizeof(Array));
    array->entries = mem_calloc(sizeof(Array), ARR_INIT_SIZE);
    array->size = ARR_INIT_SIZE;
    array->n = 0;    
    return array;
}

/**
 * Frees all memory allocated for the given array.
 *
 * This function iterates through all the entries in the array, freeing all
 * the keys and values associated with each entry. After that, it frees the
 * memory allocated for the array itself.
 *
 * @param array The array to free.
 */
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

/**
 * Resizes the array to twice its current size.
 *
 * This function is called when the load factor of the array exceeds a
 * certain threshold (75%). It allocates a new array of symbols with
 * twice the size of the current one, and rehashes all the symbols in
 * the array to their new positions in the new array. Finally, it frees
 * the old array and sets the array's size to the new size.
 *
 * @param array A pointer to the array to resize.
 */
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

/**
 * Sets a key-value pair in the array.
 *
 * This function inserts or updates a key-value pair in the given array. If the
 * key already exists, the corresponding value is updated. If the key does not
 * exist, a new entry is created and added to the array. The array is resized
 * if its load factor exceeds the defined threshold.
 *
 * @param array A pointer to the array where the key-value pair will be set.
 * @param key The key to identify the value.
 * @param value The value to be associated with the key.
 */
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

/**
 * Retrieves the value associated with a given key from the array.
 *
 * This function searches for the specified key in the array and, if found,
 * assigns the corresponding value to the output parameter. If the key is
 * not present in the array, the function returns FALSE.
 *
 * @param value A pointer to an ApexValue where the result will be stored if
 *              the key is found.
 * @param array A pointer to the array to search for the key.
 * @param key The key for which the associated value is to be retrieved.
 * @return TRUE if the key is found and the value is retrieved, otherwise FALSE.
 */
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

/**
 * Deletes a key-value pair from the array.
 *
 * This function searches for the specified key in the array and removes
 * the corresponding key-value pair if it exists. The function releases
 * the memory associated with the key and value, and adjusts the linked
 * list to maintain the array's structure. If the key is not found, the
 * array remains unchanged.
 *
 * @param array A pointer to the array from which the key-value pair
 *              will be deleted.
 * @param key The key identifying the key-value pair to remove.
 */
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

/**
 * Creates an ApexValue representing an integer.
 *
 * @param value The integer value to represent in an ApexValue.
 * @return An ApexValue of type APEX_VAL_INT with the specified value.
 */
ApexValue apexVal_makeint(int value) {
    ApexValue v;
    v.type = APEX_VAL_INT;
    v.intval = value;
    v.refcount = 1;
    return v;
}

/**
 * Creates an ApexValue representing a float.
 *
 * @param value The float value to represent in an ApexValue.
 * @return An ApexValue of type APEX_VAL_FLT with the specified value.
 */
ApexValue apexVal_makeflt(float value) {
    ApexValue v;
    v.type = APEX_VAL_FLT;
    v.fltval = value;
    v.refcount = 1;
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
    v.refcount = 1;
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

/**
 * Creates an ApexValue representing a function pointer.
 *
 * This function allocates a new ApexValue with the given function pointer
 * and type APEX_VAL_FN. The function pointer is stored in the ApexValue's
 * fnval field.
 *
 * @param fn The function pointer to associate with the ApexValue.
 *
 * @return An ApexValue with the given function pointer.
 */
ApexValue apexVal_makefn(Fn *fn) {
    ApexValue v;
    v.type = APEX_VAL_FN;
    v.fnval = fn;
    v.refcount = 1;
    return v;
}

/**
 * Creates an ApexValue representing an array.
 *
 * This function allocates a new ApexValue with the given array and type
 * APEX_VAL_ARR. The array is stored in the ApexValue's arrval field.
 *
 * @param arr The array to associate with the ApexValue.
 *
 * @return An ApexValue with the given array.
 */
ApexValue apexVal_makearr(Array *arr) {
    ApexValue v;
    v.type = APEX_VAL_ARR;
    v.arrval = arr;
    v.refcount = 1;
    return v;
}

/**
 * Creates an ApexValue with the given null value.
 *
 * This function allocates a new ApexValue with the type APEX_VAL_NULL.
 * The ApexValue is initialized with a reference count of one.
 *
 * @return An ApexValue with the null value.
 */
ApexValue apexVal_makenull(void) {
    ApexValue v;
    v.type = APEX_VAL_NULL;
    v.refcount = 1;
    return v;
}

/**
 * Frees any memory allocated for an ApexValue.
 *
 * This function is used to free any memory allocated for an ApexValue
 * when it is no longer needed. It checks the type of the ApexValue and
 * frees the appropriate memory. If the ApexValue is a function, it
 * frees the memory allocated for the function pointer and the parameters.
 * If the ApexValue is an array, it calls apexVal_freearray to free the
 * memory allocated for the array.
 *
 * @param value The ApexValue whose memory should be freed.
 */
void free_value(ApexValue value) {
    if (value.type == APEX_VAL_FN) {
        free(value.fnval->params);
        free(value.fnval);        
    } else if (value.type == APEX_VAL_ARR) {
        apexVal_freearray(value.arrval);
    }
}