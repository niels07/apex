#ifndef STRING_H
#define STRING_H

#include <stdio.h>
#include "value.h"

extern void init_string_table(void);
extern char *apexStr_new(const char *str, size_t n);
extern char *apexStr_readline(FILE *stream);
extern char *apexStr_valtostr(ApexValue value);
extern char *cat_string(const char *str1, const char *str2);
extern void free_string_table(void);

#endif