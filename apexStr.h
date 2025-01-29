#ifndef STRING_H
#define STRING_H

#include <stdio.h>

/**
 * @brief A structure representing a string in the Apex language.
 *
 * This structure represents a string in the Apex language. It contains the
 * string's value and length, as well as a pointer to the next string in the
 * linked list.
 */
typedef struct ApexString {
    /**
     * @brief The string's value.
     */
    char *value;
    /**
     * @brief The string's length.
     */
    size_t len;
    /**
     * @brief A pointer to the next string in the linked list.
     */
    struct ApexString *next;
} ApexString;

#include "apexVal.h"

#define apexStr_val(str, n) apexStr_new(str, n)->value

extern void apexStr_inittable(void);
extern ApexString *apexStr_new(const char *str, size_t len);
extern ApexString *apexStr_save(char *str, size_t len);
extern ApexString *apexStr_cat(ApexString *str1, ApexString *str2);
extern void apexStr_freetable(void);

#endif