#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "hash.h"
#include "vm.h"
#include "error.h"
#include "malloc.h"
#include "api.h"

void ApexApi_register(ApexVm *vm, const char *name, ApexApi_function func) {
    ApexApi_entry *entry;
    if (vm->api_entry_count >= MAX_API_ENTRIES) {
        ApexError_out_of_bounds("Max API functions reached");
    }
    entry = &vm->api_entries[vm->api_entry_count++];
    strncpy(entry->name, name, sizeof(entry->name));
    entry->func = func;
}

