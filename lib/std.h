#ifndef APEX_STD_H
#define APEX_STD_H

#include "vm.h"

extern void std_int(ApexVM *vm);
extern void std_str(ApexVM *vm);
extern void std_flt(ApexVM *vm);
extern void std_dbl(ApexVM *vm);
extern void std_bool(ApexVM *vm);

#endif