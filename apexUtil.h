#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

extern unsigned int apexUtil_hash(const char *str);
extern bool apexUtil_stoi(int *out, const char *str);
extern bool apexUtil_stof(float *out, const char *str);
extern bool apexUtil_stod(double *out, const char *str);
extern ApexString *apexUtil_readline(FILE *stream);

#endif