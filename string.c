#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "string.h"
#include "mem.h"
#include "value.h"
#include "util.h"


#define TABLE_SIZE 1024

static ApexString *string_table[TABLE_SIZE];

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
 * This function takes a string and its length as input and attempts to find an
 * existing entry in the string table with the same length and contents. If
 * such an entry is found, the input string is freed and the existing entry is
 * returned. If no such entry is found, a new entry is created in the string
 * table with the given string and length, and the new entry is returned.
 *
 * @param str The input string to be saved in the string table.
 * @param n The length of the input string.
 * @return A pointer to the ApexString structure representing the saved string
 *         in the string table.
 */
ApexString *apexStr_save(char *str, size_t n) {    
    unsigned int index = hash_string(str, n);
    ApexString *entry = string_table[index];

    while (entry != NULL) {
        if (entry->len == n && strncmp(entry->value, str, n) == 0) {
            free(str);
            return entry;
        }
        entry = entry->next;
    }

    ApexString *new_entry = apexMem_alloc(sizeof(ApexString));
    new_entry->value = str;
    new_entry->len = n;

    new_entry->next = string_table[index];
    string_table[index] = new_entry;

    return new_entry;
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
ApexString *apexStr_new(const char *str, size_t n) {
    unsigned int index = hash_string(str, n);
    ApexString *entry = string_table[index];
    
    while (entry != NULL) {
        if (entry->len == n && strncmp(entry->value, str, n) == 0) {
            return entry;
        }
        entry = entry->next;
    }

    ApexString *new_entry = apexMem_alloc(sizeof(ApexString));
    new_entry->value = apexMem_alloc(n + 1);
    memcpy(new_entry->value, str, n);
    new_entry->value[n] = '\0';
    new_entry->len = n;

    new_entry->next = string_table[index];
    string_table[index] = new_entry;

    return new_entry;
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
ApexString *apexStr_cat(ApexString *str1, ApexString *str2) {
    char *concat = apexMem_alloc(str1->len + str2->len + 1);
    ApexString *result;

    memcpy(concat, str1->value, str1->len);
    memcpy(concat + str1->len, str2->value, str2->len);
    concat[str1->len + str2->len] = '\0';

    result = apexStr_new(concat, str1->len + str2->len);
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
        ApexString *entry = string_table[i];
        while (entry != NULL) {
            ApexString *next = entry->next;
            free(entry->value);
            free(entry);
            entry = next;
        }
        string_table[i] = NULL;
    }
}