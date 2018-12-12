#include "vm.h"
#include <apex/api.h>

static ApexVm *api_vm;

static ApexValue *apex_api_pop(void) {
    return apex_vm_pop(api_vm);    
}

static ApexValue *apex_api_top(void) {
    return apex_vm_top(api_vm);
}

void apex_api_init(ApexVm *vm) {
    api_vm = vm;
}

void apex_api_init_methods(struct ApexApiMethods *methods) {
    methods->pop = apex_api_pop;
    methods->top = apex_api_top;    
}