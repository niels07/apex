#ifndef STRING_H
#define STRING_H

#include <stdio.h>

typedef struct ApexString {
    char *value;
    size_t len;
    struct ApexString *next;
} ApexString;

#include "value.h"

#define apexStr_val(str, n) apexStr_new(str, n)->value

extern void init_string_table(void);
extern ApexString *apexStr_new(const char *str, size_t n);
extern ApexString *apexStr_save(char *str, size_t n);
extern ApexString *apexStr_cat(ApexString *str1, ApexString *str2);
extern void free_string_table(void);

#endif