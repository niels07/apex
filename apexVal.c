#include <string.h>
#include <stdlib.h>
#include "apexVal.h"
#include "vm.h"
#include "mem.h"
#include "apexStr.h"
#include "error.h"

/**
 * Returns a string representation of an ApexValue type.
 *
 * This function takes an ApexValue and returns a string representation of its
 * type. The strings are as follows:
 * - int: APEX_VAL_INT
 * - flt: APEX_VAL_FLT
 * - dbl: APEX_VAL_DBL
 * - str: APEX_VAL_STR
 * - bool: APEX_VAL_BOOL
 * - fn: APEX_VAL_FN
 * - arr: APEX_VAL_ARR
 * - cfn: APEX_VAL_CFN
 * - type: APEX_VAL_TYPE
 * - obj: APEX_VAL_OBJ
 * - ptr: APEX_VAL_PTR
 * - null: APEX_VAL_NULL
 *
 * @param value The ApexValue to get a string representation of its type.
 *
 * @return A string representation of the ApexValue type.
 */
const char *apexVal_typestr(ApexValue value) {
    switch (value.type) {
    case APEX_VAL_INT:
        return "int";
    case APEX_VAL_FLT:
        return "flt";
    case APEX_VAL_DBL:
        return "dbl";
    case APEX_VAL_STR:
        return "str";
    case APEX_VAL_BOOL:
        return "bool";
    case APEX_VAL_FN:
        return "fn";
    case APEX_VAL_ARR:
        return "arr";
    case APEX_VAL_CFN:
        return "cfn";
    case APEX_VAL_TYPE:
        return "type";
    case APEX_VAL_OBJ:
        return "obj";
    case APEX_VAL_PTR:
        return "ptr";
    case APEX_VAL_NULL:
        return "null";
    }
    return "null";
}

#define ARR_INIT_SIZE 16
#define OBJ_INIT_SIZE 16
#define ARR_LOAD_FACTOR 0.75
#define OBJ_LOAD_FACTOR 0.75

static unsigned int hash_string(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash << 5) + hash + (unsigned char)(*str);
        str++;
    }
    return hash;
}

/**
 * Converts a Fn to its string representation.
 *
 * This function takes a pointer to a Fn and returns a string representation of
 * the function, formatted as "[function <name> at addr <addr>]".
 * @param fn The pointer to the Fn to convert.
 * @return A char pointer to the string representation of the function.
 */
static char *fntostr(Fn *fn) {
    char addrstr[20];
    snprintf(addrstr, sizeof(addrstr), "%d", fn->addr);
    size_t n = 20 + strlen(fn->name) + strlen(addrstr);
    char *str = apexMem_alloc(n + 1);
    snprintf(str, n + 1, "[function %s at addr %d]", fn->name, fn->addr);
    return apexStr_save(str, n)->value;
}

/**
 * Converts an ApexCfn to its string representation.
 *
 * This function takes a C function pointer and returns a string
 * representation formatted as "[cfunction <address>]".
 *
 * @param fn The ApexCfn function pointer to convert.
 * @return A char pointer to the string representation of the C function.
 */
static char *cfntostr(ApexCfn fn) {
    size_t size = 32 + strlen(fn.name);
    char *str = apexMem_alloc(size + 1);
    snprintf(str, size, "[cfunction %s: %p]", fn.name, fn.fn);
    return apexStr_save(str, size)->value;
}

/**
 * Converts a pointer to its string representation.
 *
 * This function takes a pointer and returns a string representation
 * of the pointer, formatted as "[pointer <address>]".
 *
 * @param ptr The pointer to convert to a string representation.
 * @return A char pointer to the string representation of the pointer.
 */
static char *ptrtostr(void *ptr) {
    char *str = apexMem_alloc(32);
    snprintf(str, 32, "[pointer %p]", ptr);
    return apexStr_save(str, 20)->value;
}

/**
 * Converts an ApexObject type to its string representation.
 *
 * This function takes a pointer to an ApexObject and returns a string
 * representation of the type, formatted as "[type <name>]"
 *
 * @param obj The pointer to the ApexObject to convert.
 * @return A char pointer to the string representation of the type.
 */
static char *typetostr(ApexObject *obj) {
    size_t len = strlen(obj->name) + 9;
    char *str = apexMem_alloc(len + 1);
    snprintf(str, len + 1, "[type %s]", obj->name);
    return apexStr_save(str, len)->value;
}


/**
 * Converts an ApexObject instance to its string representation.
 *
 * This function takes a pointer to an ApexObject and returns a string
 * representation of the instance, formatted as "[instance of <name>]"
 *
 * @param obj The pointer to the ApexObject to convert.
 * @return A char pointer to the string representation of the instance.
 */
static char *objtostr(ApexObject *obj) {
    size_t len = strlen(obj->name) + 14;
    char *str = apexMem_alloc(len + 1);
    snprintf(str, len + 1, "[instance of %s]", obj->name);
    return apexStr_save(str, len)->value;
}

/**
 * Converts an Array to its string representation.
 *
 * This function takes a pointer to an Array and converts it to a string
 * representation, formatted as a list of key-value pairs. The string
 * representation starts and ends with square brackets, with key-value pairs
 * separated by commas. Each key-value pair is represented as "key => value".
 * String keys and values are enclosed in double quotes, while other types are
 * represented by their respective string conversions.
 *
 * If the array is empty, the function returns a string containing "[]".
 *
 * @param arr A pointer to the Array to convert to a string.
 * @return A char pointer to the string representation of the Array.
 */
static char *arrtostr(Array *arr) {
    char *str = apexMem_alloc(128);
    int n = 1; 
    int size = 128;
    
    if (arr->entry_count == 0) {
        snprintf(str, size, "[]");
        return apexStr_save(str, 2)->value;
    }

    str[0] = '[';
    str[1] = '\0';

    for (int i = 0; i < arr->entry_size; i++) {
        ArrayEntry *entry = arr->entries[i];
        while (entry) {
            char *keystr = apexVal_tostr(entry->key);
            char *valstr = apexVal_tostr(entry->value);

            int keylen = strlen(keystr);
            int vallen = strlen(valstr);
            bool is_key_string = (entry->key.type == APEX_VAL_STR);
            bool is_val_string = (entry->value.type == APEX_VAL_STR);

            int elen = (is_key_string ? 2 : 0) + keylen + 4 +
                       (is_val_string ? 2 : 0) + vallen;
            int nspace = n + elen + (i > 0 ? 2 : 0);

            if (nspace >= size) {
                while (nspace >= size) {
                    size *= 2;
                }
                str = apexMem_realloc(str, size);
            }

            if (n > 1) {
                strcat(str, ", ");
                n += 2;
            }
            if (is_key_string) {
                str[n++] = '"';
                strncpy(&str[n], keystr, keylen);
                n += keylen;
                str[n++] = '"';
            } else {
                strncpy(&str[n], keystr, keylen);
                n += keylen;
            }
            memcpy(&str[n], " => ", 4);
            n += 4;
            if (is_val_string) {
                str[n++] = '"';
                strncpy(&str[n], valstr, vallen);
                n += vallen;
                str[n++] = '"';
            } else {
                strncpy(&str[n], valstr, vallen);
                n += vallen;
            }
            str[n] = '\0';
            entry = entry->next;
        }
    }

    if (n + 1 >= size) {
        size += 1;
        str = apexMem_realloc(str, size);
    }
    str[n++] = ']';
    str[n] = '\0';

    return apexStr_save(str, n)->value;
}

/**
 * Converts an ApexValue to a string representation.
 *
 * This function takes an ApexValue and returns a string representation based
 * on its type. The conversion is performed as follows:
 * - Integers are converted to their decimal representation.
 * - Floats and doubles are converted to their string representation with
 *   limited precision.
 * - Strings return their underlying char pointer.
 * - Booleans are converted to "true" or "false".
 * - Functions and arrays are converted using their respective conversion
 *   functions.
 * - Null values are converted to the string "null".
 *
 * @param value The ApexValue to convert to a string.
 * @return A char pointer to the string representation of the ApexValue.
 */
char *apexVal_tostr(ApexValue value) {
    switch (value.type) {
    case APEX_VAL_INT: {
        char buf[12];
        sprintf(buf, "%d", value.intval);
        return apexStr_val(buf, strlen(buf));
    }
    case APEX_VAL_FLT: {
        char buf[48];
        sprintf(buf, "%.8g", apexVal_flt(value));
        return apexStr_val(buf, strlen(buf));
    }
    case APEX_VAL_DBL: {
        char buf[250];
        sprintf(buf, "%.17g", apexVal_dbl(value));
        return apexStr_val(buf, strlen(buf));
    }
    case APEX_VAL_STR:
        return value.strval->value;

    case APEX_VAL_BOOL:
        return apexStr_val(value.boolval ? "true" : "false", value.boolval ? 4 : 5);

    case APEX_VAL_FN:
        return fntostr(value.fnval);

    case APEX_VAL_CFN:
        return cfntostr(value.cfnval);

    case APEX_VAL_ARR:
        return arrtostr(value.arrval);

    case APEX_VAL_OBJ:
        return objtostr(value.objval);

    case APEX_VAL_TYPE:
        return typetostr(value.objval);

    case APEX_VAL_PTR:
        return ptrtostr(value.ptrval);

    case APEX_VAL_NULL:
        return apexStr_val("null", 4);
    }
    return NULL;
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
        return hash_string(key.strval->value);
    case APEX_VAL_BOOL:
        return (unsigned int)key.boolval;
    case APEX_VAL_FLT: {
        union { float f; unsigned int i; } flt_to_int;
        flt_to_int.f = key.fltval;
        return flt_to_int.i;
    }
    case APEX_VAL_DBL: {
        union { double d; unsigned int i; } dbl_to_int;
        dbl_to_int.d = key.dblval;
        return dbl_to_int.i;
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
 * If the types of the two values do not match, the function returns false.
 *
 * @param a The first ApexValue to compare.
 * @param b The second ApexValue to compare.
 * 
 * @return true if the two ApexValue objects are equal, otherwise false.
 */
static bool value_equals(const ApexValue a, const ApexValue b) {
    if (a.type != b.type) {
        return false;
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
    case APEX_VAL_DBL:
        return a.dblval == b.dblval;
    case APEX_VAL_NULL:
        return true;
    default:
        return false;
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
void apexVal_retain(ApexValue value) {
    switch (value.type) {
    case APEX_VAL_ARR:
        value.arrval->refcount++;
        break;

    case APEX_VAL_FN:
        value.fnval->refcount++;
        break;

    case APEX_VAL_OBJ:
        value.objval->refcount++;
        break;

    default:
        break;
    }
}

/**
 * Releases the memory associated with an ApexValue.
 *
 * This function decrements the reference count of the given ApexValue
 * if it is of type array or function. If the reference count reaches zero,
 * it frees the memory associated with the array or function and its contents.
 *
 * @param value The ApexValue whose memory is to be released.
 */
void apexVal_release(ApexValue value) {
    switch (value.type) {
    case APEX_VAL_ARR: {
        int refcount = --value.arrval->refcount;
        if (refcount <= 0) {
            apexVal_freearray(value.arrval);
        }
        break;
    }
    case APEX_VAL_FN: {
        int refcount = --value.fnval->refcount;
        if (refcount <= 0) {
            free(value.fnval->params);
            free(value.fnval);
        }
        break;
    }
    case APEX_VAL_TYPE:
    case APEX_VAL_OBJ: {
        int refcount = --value.objval->refcount;
        if (refcount <= 0) {
            apexVal_freeobject(value.objval);
        }
        break;
    }
    default:
        break;
    }
}

/**
 * Initializes a new function with the given name, parameters, and address.
 *
 * This function allocates a new Fn structure and assigns it the given name,
 * parameters, and address. The reference count of the new function is set to
 * zero.
 *
 * @param name The name of the new function.
 * @param params A const char ** containing the parameter names of the new
 *               function.
 * @param argc The number of parameters in the new function.
 * @param addr The address in the instruction chunk of the new function.
 * @return A pointer to the newly allocated Fn.
 */
Fn *apexVal_newfn(const char *name, const char **params, int argc, int addr) {
    Fn *fn = apexMem_alloc(sizeof(Fn));
    fn->name = name;
    fn->argc = argc;
    fn->params = params;
    fn->addr = addr;
    fn->refcount = 0;
    return fn;
}

/**
 * Initializes a new ApexCfn structure with the given name, argument count,
 * and function pointer.
 *
 * This function allocates a new ApexCfn structure and assigns it the given
 * name, argument count, and function pointer.
 *
 * @param name The name of the new ApexCfn.
 * @param argc The number of arguments expected by the new ApexCfn.
 * @param fn The function pointer of the new ApexCfn.
 * @return A pointer to the newly allocated ApexCfn.
 */
ApexCfn apexVal_newcfn(char *name, int argc, int (*fn)(ApexVM *)) {
    ApexCfn cfn;
    cfn.name = name;
    cfn.argc = argc;
    cfn.fn = fn;
    return cfn;
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
    Array *array = apexMem_alloc(sizeof(Array));
    array->entries = apexMem_calloc(sizeof(ArrayEntry), ARR_INIT_SIZE);
    array->iter = apexMem_calloc(sizeof(ArrayEntry), ARR_INIT_SIZE);
    array->entry_size = ARR_INIT_SIZE;
    array->iter_size = ARR_INIT_SIZE;
    array->entry_count = 0;
    array->iter_count = 0;
    array->refcount = 0;
    return array;
}

/**
 * Creates a new object with the given name.
 *
 * This function allocates memory for a new ApexObject and initializes
 * its fields. The object is given an initial size, and its reference
 * count is set to one. The entries in the object are initialized based
 * on the defined initial size.
 *
 * @param name The name to assign to the new object.
 * @return A pointer to the newly created ApexObject.
 */
ApexObject *apexVal_newobject(const char *name) {
    ApexObject *object = apexMem_alloc(sizeof(ApexObject));
    object->entries = apexMem_calloc(sizeof(ApexObjectEntry), OBJ_INIT_SIZE);
    object->size = OBJ_INIT_SIZE;
    object->n = 0;
    object->refcount = 0;
    object->name = name;
    return object;
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
    for (int i = 0; i < array->entry_size; i++) {
        ArrayEntry *entry = array->entries[i];
        while (entry) {
            ArrayEntry *next = entry->next;
            apexVal_release(entry->key);
            apexVal_release(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(array->entries);
    free(array->iter);
    free(array);
}

/**
 * Frees all memory allocated for the given object.
 *
 * This function iterates through all the entries in the object, freeing all
 * the values associated with each entry. After that, it frees the memory
 * allocated for the object itself.
 *
 * @param object The object to free.
 */
void apexVal_freeobject(ApexObject *object) {
    for (int i = 0; i < object->size; i++) {
        ApexObjectEntry *entry = object->entries[i];
        while (entry) {
            ApexObjectEntry *next = entry->next;
            apexVal_release(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(object->entries);
    free(object);
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
static void array_resize_entries(Array *array) {
    int new_size = array->entry_size * 2;
    ArrayEntry **new_entries = apexMem_calloc(new_size, sizeof(ApexObjectEntry *));
    for (int i = 0; i < array->entry_size; i++) {
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
    array->entry_size = new_size;
}

/**
 * Resizes the array's iterator to twice its current size.
 *
 * This function is called when the number of elements in the array exceeds
 * the current size of the iterator. It allocates a new array of symbols with
 * twice the size of the current one, and copies the existing elements to
 * the new array. Finally, it frees the old array and sets the array's
 * iterator size to the new size.
 *
 * @param array A pointer to the array whose iterator is to be resized.
 */
static void array_resize_iter(Array *array) {
    int new_size = array->iter_size * 2;
    ArrayEntry **new_iter = apexMem_calloc(new_size, sizeof(ApexObjectEntry *));
    for (int i = 0; i < array->iter_count; i++) {
        new_iter[i] = array->iter[i];
    }
    free(array->iter);
    array->iter = new_iter;
    array->iter_size = new_size;
}

/**
 * Resizes the object to twice its current size.
 *
 * This function is called when the load factor of the object exceeds a
 * certain threshold (75%). It allocates a new array of entries with
 * twice the size of the current one, and rehashes all the entries in
 * the object to their new positions in the new array. Finally, it frees
 * the old array and sets the object's size to the new size.
 *
 * @param object A pointer to the object to resize.
 */
static void object_resize(ApexObject *object) {
    int new_size = object->size * 2;
    ApexObjectEntry **new_entries = apexMem_calloc(new_size, sizeof(ApexObjectEntry *));
    for (int i = 0; i < object->size; i++) {
        ApexObjectEntry *entry = object->entries[i];
        while (entry) {
            unsigned int index = hash_string(entry->key) % new_size;
            ApexObjectEntry *next = entry->next;
            entry->next = new_entries[index];
            new_entries[index] = entry;
            entry = next;
        }
    }
    free(object->entries);
    object->entries = new_entries;
    object->size = new_size;
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
    unsigned int index = get_array_index(key) % array->entry_size;
    ArrayEntry *entry = array->entries[index];

    while (entry) {
        if (value_equals(entry->key, key)) {
            apexVal_release(entry->value);
            apexVal_retain(value);
            array->iter[entry->index]->value = value;
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    entry = apexMem_alloc(sizeof(ArrayEntry));
    apexVal_retain(key); 
    apexVal_retain(value);
    entry->key = key;
    entry->value = value;
    entry->next = array->entries[index];
    array->entries[index] = entry;
    if (entry->key.type == APEX_VAL_INT) {
         printf("setting iter at %d to (key: %d, value: %d)\n", array->iter_count, entry->key.intval, entry->value.intval);
    } else {
        printf("setting iter at %d to (key: %s, value: %d)\n", array->iter_count, entry->key.strval->value, entry->value.intval);
    }
    array->iter[array->iter_count++] = entry;
    array->entry_count++;

    if ((float)array->entry_count / array->entry_size > ARR_LOAD_FACTOR) {
        array_resize_entries(array);
    }
    if ((float)array->iter_count / array->iter_size > ARR_LOAD_FACTOR) {
        array_resize_iter(array);
    }
}

/**
 * Sets a key-value pair in the object.
 *
 * This function inserts or updates a key-value pair in the given object. If the
 * key already exists, the corresponding value is updated. If the key does not
 * exist, a new entry is created and added to the object. The object is resized
 * if its load factor exceeds the defined threshold.
 *
 * @param object A pointer to the object where the key-value pair will be set.
 * @param key The key to identify the value.
 * @param value The value to be associated with the key.
 */
void apexVal_objectset(ApexObject *object, const char *key, ApexValue value) {
    unsigned int index = hash_string(key) % object->size;
    ApexObjectEntry *entry = object->entries[index];

    while (entry) {
        if (entry->key == key) {
            apexVal_release(entry->value);
            apexVal_retain(value);
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    entry = apexMem_alloc(sizeof(ApexObjectEntry));
    apexVal_retain(value);
    entry->key = key;
    entry->value = value;
    entry->next = object->entries[index];
    object->entries[index] = entry;
    object->n++;

    if ((float)object->n / object->size > OBJ_LOAD_FACTOR) {
        object_resize(object);
    }
}

/**
 * Creates a deep copy of an object.
 *
 * This function creates a new object with the same name as the given object.
 * It iterates through all the entries in the given object and inserts or
 * updates the corresponding key-value pair in the new object. If the value is
 * of type object, it recursively calls apexVal_objectcpy to create a deep
 * copy of the nested object. Finally, it returns the new object.
 *
 * @param object A pointer to the object to copy.
 * @return A pointer to the new object created as a deep copy of the given
 *         object.
 */
ApexObject *apexVal_objectcpy(ApexObject *object) {
    ApexObject *newobj = apexVal_newobject(object->name);
    for (int i = 0; i < object->size; i++) {
        ApexObjectEntry *entry = object->entries[i];
        if (entry) {
            ApexValue value = entry->value;
            if (value.type == APEX_VAL_OBJ) {
                ApexObject *objcpy = apexVal_objectcpy(entry->value.objval);
                value = apexVal_makeobj(objcpy);
            }
            apexVal_objectset(newobj, entry->key, value);
        }
    }
    return newobj;
}

/**
 * Retrieves the value associated with a given key from the array.
 *
 * This function searches for the specified key in the array and, if found,
 * assigns the corresponding value to the output parameter. If the key is
 * not present in the array, the function returns false.
 *
 * @param value A pointer to an ApexValue where the result will be stored if
 *              the key is found.
 * @param array A pointer to the array to search for the key.
 * @param key The key for which the associated value is to be retrieved.
 * @return true if the key is found and the value is retrieved, otherwise false.
 */
bool apexVal_arrayget(ApexValue *value, Array *array, const ApexValue key) {
    unsigned int index = get_array_index(key) % array->entry_size;
    ArrayEntry *entry = array->entries[index];

    while (entry) {
        if (value_equals(entry->key, key)) {
            *value = entry->value;
            return true;
        }
        entry = entry->next;
    }

    return false;
}

/**
 * Retrieves the value associated with a given key from the object.
 *
 * This function searches for the specified key in the object and, if found,
 * assigns the corresponding value to the output parameter. If the key is
 * not present in the object, the function returns false.
 *
 * @param value A pointer to an ApexValue where the result will be stored if
 *              the key is found.
 * @param object A pointer to the object to search for the key.
 * @param key The key for which the associated value is to be retrieved.
 * @return true if the key is found and the value is retrieved, otherwise false.
 */
bool apexVal_objectget(ApexValue *value, ApexObject *object, const char *key) {
    unsigned int index = hash_string(key) % object->size;
    ApexObjectEntry *entry = object->entries[index];
    
    while (entry) {
        if (entry->key == key) {
            *value = entry->value;
            return true;
        }
        entry = entry->next;
    }

    return false;
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
    unsigned int index = get_array_index(key) % array->entry_size;
    ArrayEntry *prev = NULL;
    ArrayEntry *entry = array->entries[index];

    while (entry) {
        if (value_equals(entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                array->entries[index] = entry->next;
            }
            apexVal_release(entry->key);
            apexVal_release(entry->value);
            free(entry);
            array->entry_count--;
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
    return v;
}

/**
 * Creates an ApexValue representing a double.
 *
 * @param value The double value to represent in an ApexValue.
 * @return An ApexValue of type APEX_VAL_DBL with the specified value.
 */
ApexValue apexVal_makedbl(double value) {
    ApexValue v;
    v.type = APEX_VAL_DBL;
    v.dblval = value;
    return v;
}

/**
 * Creates an ApexValue with the given string value.
 *
 * @param value The string value to associate with the ApexValue.
 *
 * @return An ApexValue with the given string value.
 */
ApexValue apexVal_makestr(ApexString *value) {
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
ApexValue apexVal_makebool(bool value) {
    ApexValue v;
    v.type = APEX_VAL_BOOL;
    v.boolval = value;
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
    return v;
}


/**
 * Creates an ApexValue representing a type (i.e. a collection of functions and
 * variables that can be used to create objects).
 *
 * @param obj The object to associate with the ApexValue.
 *
 * @return An ApexValue with the given object type.
 */
ApexValue apexVal_maketype(ApexObject *obj) {
    ApexValue v;
    v.type = APEX_VAL_TYPE;
    v.objval = obj;
    return v;
}

/**
 * Creates an ApexValue representing an object.
 *
 * This function allocates a new ApexValue with the given object and type
 * APEX_VAL_OBJ. The object is stored in the ApexValue's objval field.
 *
 * @param obj The object to associate with the ApexValue.
 *
 * @return An ApexValue with the given object.
 */
ApexValue apexVal_makeobj(ApexObject *obj) {
    ApexValue v;
    v.type = APEX_VAL_OBJ;
    v.objval = obj;
    return v;
}

/**
 * Creates an ApexValue representing a foreign function.
 *
 * This function allocates a new ApexValue with the given foreign function
 * and type APEX_VAL_CFN. The foreign function is stored in the ApexValue's
 * cfnval field.
 *
 * @param cfn The foreign function to associate with the ApexValue.
 *
 * @return An ApexValue with the given foreign function.
 */
ApexValue apexVal_makecfn(ApexCfn cfn) {
    ApexValue v;
    v.type = APEX_VAL_CFN;
    v.cfnval = cfn;
    return v;
}

/**
 * Creates an ApexValue representing a pointer.
 *
 * This function allocates a new ApexValue with the given pointer
 * and type APEX_VAL_PTR. The pointer is stored in the ApexValue's
 * ptrval field.
 *
 * @param ptr The pointer to associate with the ApexValue.
 *
 * @return An ApexValue with the given pointer.
 */
ApexValue apexVal_makeptr(void *ptr) {
    ApexValue v;
    v.type = APEX_VAL_PTR;
    v.ptrval = ptr;
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
    return v;
}

/**
 * Retrieves the length of an array.
 *
 * This function takes an ApexValue of type array and returns the number of
 * elements in the array.
 *
 * @param value The ApexValue of type array to retrieve the length of.
 *
 * @return The length of the array.
 */
int apexVal_arrlen(ApexValue value) {
    Array *arr = value.arrval;
    return arr->entry_count;
}

/**
 * Converts an ApexValue to a boolean representation.
 *
 * This function takes an ApexValue and returns a boolean representation
 * based on its type and value. The conversion is performed as follows:
 * - Integers are considered true if non-zero.
 * - Floats and doubles are considered true if non-zero.
 * - Booleans retain their value.
 * - Strings are considered true if not null.
 * - Functions, arrays, types, and objects are always considered true.
 * - Null values are considered false.
 *
 * @param value The ApexValue to convert to a boolean.
 * @return A boolean representation of the ApexValue.
 */
bool apexVal_tobool(ApexValue value) {
    switch (value.type) {
    case APEX_VAL_INT:
        value = apexVal_makebool(value.intval != 0);

    case APEX_VAL_FLT:
        value = apexVal_makebool(value.fltval != 0);
        break;

    case APEX_VAL_DBL:
        value = apexVal_makebool(value.dblval != 0);
        break;

    case APEX_VAL_BOOL:
        break;

    case APEX_VAL_STR:
        value = apexVal_makebool(value.strval != NULL);
        break;

    case APEX_VAL_FN:
    case APEX_VAL_ARR:
    case APEX_VAL_TYPE:
    case APEX_VAL_OBJ:
    case APEX_VAL_CFN:
    case APEX_VAL_PTR:
        value = apexVal_makebool(true);
        break;

    case APEX_VAL_NULL:
        value = apexVal_makebool(false);
        break;
    }
    return value.boolval;
}