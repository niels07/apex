#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "mem.h"
#include "value.h"
#include "util.h"


#define TABLE_SIZE 1024

typedef struct String {
    char *value;
    size_t len;
    struct String *next;
} String;

static String *string_table[TABLE_SIZE];

/**
 * Computes a hash value for a given string.
 *
 * This function takes a string and its length as input and calculates a hash value
 * using a combination of bitwise operations and modular arithmetic. The hash value
 * is computed by iteratively processing each character in the string with a specified
 * step size. The resulting hash value is then reduced modulo TABLE_SIZE to fit within
 * the hash table.
 *
 * @param str The input string to be hashed.
 * @param len The length of the input string.
 * @return An unsigned integer representing the hash value of the input string.
 */
unsigned int hash_string(const char *str, size_t len) {
    unsigned int h = (unsigned int)len; 
    size_t step = (len >> 5) + 1;
    size_t i;
    for (i = 0; i < len; i += step) {
        h = h ^ ((h << 5) + (h >> 2) + (unsigned char)str[i]);
    }
    return h % TABLE_SIZE;
}

/**
 * Initializes the string table.
 *
 * This function initializes the string table by setting all of its entries to
 * NULL. This is called once, at the beginning of the program.
 */
void init_string_table(void) {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        string_table[i] = NULL;
    }
}

/**
 * Saves a string in the string table.
 *
 * This function takes a string and its length as input and checks if the string
 * is already present in the string table. If the string is already present, the
 * function returns the existing pointer to the string, and frees the input string.
 * If the string is not present, a new entry is created in the string table with
 * the input string, and a pointer to the new string is returned.
 *
 * @param str The string to save in the string table.
 * @param n The length of the input string.
 * @return A pointer to the saved string in the string table.
 */
static char *save_string(char *str, size_t n) {
    unsigned int index;
    String *entry;
    String *new_entry;
    
    index = hash_string(str, n);
    entry = string_table[index];

    while (entry != NULL) {
        if (entry->len == n && strncmp(entry->value, str, n) == 0) {
            free(str);
            return entry->value;
        }
        entry = entry->next;
    }

    new_entry = mem_alloc(sizeof(String));
    new_entry->value = str;
    new_entry->len = n;

    new_entry->next = string_table[index];
    string_table[index] = new_entry;

    return new_entry->value;
}

/**
 * Creates a new string in the string table.
 *
 * This function takes a string and its length as input and creates a new entry
 * in the string table if it does not already exist. If the string is already
 * in the table, the existing pointer is returned. The new string is allocated
 * with a size of n+1 to account for the null-terminator.
 *
 * @param str The string to add to the table.
 * @param n The length of the string.
 * @return A pointer to the newly allocated string in the string table.
 */
char *apexStr_new(const char *str, size_t n) {
    unsigned int index = hash_string(str, n);
    String *entry = string_table[index];
    String *new_entry;
    
    while (entry != NULL) {
        if (entry->len == n && strncmp(entry->value, str, n) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    new_entry = mem_alloc(sizeof(String));
    new_entry->value = mem_alloc(n + 1);
    memcpy(new_entry->value, str, n);
    new_entry->value[n] = '\0';
    new_entry->len = n;

    new_entry->next = string_table[index];
    string_table[index] = new_entry;

    return new_entry->value;
}

/**
 * Converts a function pointer to a string.
 *
 * This function takes a function pointer and its parameter list as input and
 * creates a string representation of the function pointer. The string is
 * formatted as "[function <name> at addr <addr>]". If the string is already
 * in the string table, the existing pointer is returned. Otherwise, a new
 * entry is created in the string table.
 *
 * @param fn The function pointer to convert to a string.
 * @return A pointer to the string representation of the function pointer in
 *         the string table.
 */
static char *fntostr(Fn *fn) {
    char addrstr[20];
    char *str;
    size_t n;

    apexUtl_snprintf(addrstr, sizeof(addrstr), "%d", fn->addr);
    n = 20 + strlen(fn->name) + strlen(addrstr);
    str = mem_alloc(n + 1);
    apexUtl_snprintf(str, n + 1, "[function %s at addr %d]", fn->name, fn->addr);
    return save_string(str, n);
}


/**
 * Converts an array to a string.
 *
 * This function takes an array as input and creates a string representation of
 * the array. The string is formatted as "[<key> => <value>, ...]". If the
 * string is already in the string table, the existing pointer is returned.
 * Otherwise, a new entry is created in the string table.
 *
 * @param arr The array to convert to a string.
 * @return A pointer to the string representation of the array in the string
 *         table.
 */
static char *arrtostr(Array *arr) {
    char *str = mem_alloc(128);
    int n = 1; 
    int size = 128;
    int i;

    if (arr->n == 0) {
        apexUtl_snprintf(str, size, "[]");
        return save_string(str, 2);
    }

    str[0] = '[';
    str[1] = '\0';

    for (i = 0; i < arr->size; i++) {
        ArrayEntry *entry = arr->entries[i];
        while (entry) {
            char *keystr = apexStr_valtostr(entry->key);
            char *valstr = apexStr_valtostr(entry->value);

            int keylen = strlen(keystr);
            int vallen = strlen(valstr);
            Bool is_key_string = (entry->key.type == APEX_VAL_STR);
            Bool is_val_string = (entry->value.type == APEX_VAL_STR);

            int elen = (is_key_string ? 2 : 0) + keylen + 4 +
                       (is_val_string ? 2 : 0) + vallen;
            int nspace = n + elen + (i > 0 ? 2 : 0);

            if (nspace >= size) {
                while (nspace >= size) {
                    size *= 2;
                }
                str = mem_realloc(str, size);
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
        str = mem_realloc(str, size);
    }
    str[n++] = ']';
    str[n] = '\0';

    return save_string(str, n);
}

/**
 * Converts an ApexValue to a string.
 *
 * This function takes an ApexValue as input and returns a pointer to a string
 * representation of the value. The string is allocated from the string table if
 * it does not already exist. If the string is already in the table, the existing
 * pointer is returned.
 *
 * @param value The ApexValue to convert to a string.
 *
 * @return A pointer to the string representation of the ApexValue in the
 *         string table.
 */
char *apexStr_valtostr(ApexValue value) {
    switch (value.type) {
    case APEX_VAL_INT: {
        char buf[12];
        sprintf(buf, "%d", value.intval);
        return apexStr_new(buf, strlen(buf));
    }
    case APEX_VAL_FLT: {
        char buf[48];
        sprintf(buf, "%.8g", apexVal_flt(value));
        return apexStr_new(buf, strlen(buf));
        break;
    }
    case APEX_VAL_STR:
        return value.strval;

    case APEX_VAL_BOOL:
        return apexStr_new(value.boolval ? "true" : "false", value.boolval ? 4 : 5);

    case APEX_VAL_FN:
        return fntostr(value.fnval);

    case APEX_VAL_ARR:
        return arrtostr(value.arrval);

    case APEX_VAL_NULL:
        return apexStr_new("null", 4);
    }
    return NULL;
}

/**
 * Reads a line of text from the given stream and returns a pointer to it.
 *
 * This function allocates memory for the line and reads it from the stream.
 * It checks the string table to see if the line is already present. If it is,
 * the existing pointer is returned. If not, a new entry is added to the table
 * with the given string.
 *
 * @param stream The file stream to read from.
 * @return A pointer to the line of text that was read, or NULL if end of file
 *         was encountered.
 */
char *apexStr_readline(FILE *stream) {
    unsigned int index;
    String *new_entry;
    String *entry;
    size_t size = 128;
    size_t len = 0;
    char *buffer = mem_alloc(size);
    int c;

    while ((c = getc(stream)) != '\n' && c != EOF) {
        if (len + 1 >= size) {
            size *= 2;
            buffer = mem_realloc(buffer, size);
        }
        buffer[len++] = c;
    }

    if (len == 0 && c == EOF) {
        free(buffer);
        return NULL;
    }

    buffer[len] = '\0';

    index = hash_string(buffer, len);
    entry = string_table[index];
    while (entry) {
        if (entry->len == len && memcmp(entry->value, buffer, len) == 0) {
            free(buffer);
            return entry->value;
        }
        entry = entry->next;
    }

    new_entry = mem_alloc(sizeof(String));
    new_entry->value = buffer;
    new_entry->len = len;
    new_entry->next = string_table[index];
    string_table[index] = new_entry;

    return buffer;
}


/**
 * Concatenates two strings and adds the result to the string table.
 *
 * This function allocates memory for the concatenated string, copies the
 * input strings into the new memory, and then calls new_string to add the
 * new string to the string table.
 *
 * @param str1 The first string to be concatenated.
 * @param str2 The second string to be concatenated.
 * @return A pointer to the newly allocated concatenated string in the string
 *         table.
 */
char *cat_string(const char *str1, const char *str2) {
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char *concat = mem_alloc(len1 + len2 + 1);
    char *result;

    memcpy(concat, str1, len1);
    memcpy(concat + len1, str2, len2);
    concat[len1 + len2] = '\0';

    result = apexStr_new(concat, len1 + len2);
    free(concat);

    return result;
}

/**
 * Frees the memory allocated for the string table.
 *
 * This function is called once, at the end of the program, to free the memory
 * allocated for the string table. It iterates over each entry in the table,
 * freeing the memory allocated for the string itself and the String structure
 * that holds it. It then sets the entry in the table to NULL.
 */
void free_string_table(void) {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        String *entry = string_table[i];
        while (entry != NULL) {
            String *next = entry->next;
            free(entry->value);
            free(entry);
            entry = next;
        }
        string_table[i] = NULL;
    }
}