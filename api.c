
#include "api.h"
#include "vm.h"

static ApexVm *api_vm;

void apex_api_init(ApexVm *vm) {
    api_vm = vm;
}

ApexValue *apex_pop(void) {
    return apex_vm_pop(api_vm);    
}

ApexValue *apex_top(void) {
    return apex_vm_top(api_vm);
}
