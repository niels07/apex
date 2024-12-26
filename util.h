#ifndef UTIL_H
#define UTIL_H

#include "bool.h"

extern Bool apexUtl_stoi(int *out, const char *str);
extern Bool apexUtl_stof(float *out, const char *str);
extern int apexUtl_snprintf(char *buffer, size_t size, const char *format, ...);
#endif