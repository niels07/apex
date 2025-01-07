#include "vm.h"
#include "value.h"
#include "util.h"
#include "lib.h"

void io_write(ApexVM *vm) {
    ApexValue value = apexVM_pop(vm);
    printf("%s", apexVal_tostr(value));
}

void io_print(ApexVM *vm) {
    io_write(vm);
    printf("\n");
}

void io_read(ApexVM *vm) {
    ApexString *line = apexUtl_readline(stdin);
    apexVM_pushval(vm, apexVal_makestr(line));
}

apex_reglib(io,
    apex_regfn( "write", io_write ),
    apex_regfn( "print", io_print ),
    apex_regfn( "read",  io_read  )
);