#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

extern bool apexUtl_stoi(int *out, const char *str);
extern bool apexUtl_stof(float *out, const char *str);
extern bool apexUtl_stod(double *out, const char *str);
extern ApexString *apexUtl_readline(FILE *stream);

#endif