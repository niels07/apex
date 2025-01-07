#ifndef IO_H
#define IO_H

#include "vm.h"

extern void io_write(ApexVM *vm);
extern void io_print(ApexVM *vm);
extern void io_read(ApexVM *vm);

#endif