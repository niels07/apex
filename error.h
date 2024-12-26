#ifndef ERROR_H
#define ERROR_H

#include "vm.h"
#include "srcloc.h"

extern void apexErr_print(const char *fmt, ...);
extern void apexErr_fatal(SrcLoc srcloc, const char *fmt, ...);
extern void apexErr_syntax(SrcLoc srcloc, const char *fmt, ...);
void apexErr_runtime(ApexVM *vm, const char *fmt, ...);

#endif