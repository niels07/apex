#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <apex/value.h>
#include "vm.h"
#include "malloc.h"
#include "error.h"

#define STACK_INIT_SIZE 256

#define TOP(vm, i) ((vm)->stack_top - i - 1)

struct ApexVm {
   ApexVmCode code; 
   ApexByte *p;
   ApexValue *stack;
   ApexValue *stack_top;
   size_t stack_size;
   size_t stack_n;
   int debug;
   ApexHashTable *table;
};

static void load_code(ApexVm *vm, const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (!file) {
        apex_error_io("could not open file: %s", strerror(errno));
    }

    if (fread(&vm->code.n, sizeof(size_t),  1, file) == 0) {
        fclose(file);
        apex_error_io("read failed: %s", strerror(errno));
    }
    vm->code.bytes = apex_malloc(vm->code.n);

    if (fread(vm->code.bytes, vm->code.n, 1, file) == 0) {
        fclose(file);
        apex_error_io("read failed: %s", strerror(errno));
    }
    fclose(file);
}

static void invalid_operation1(const char *opr, ApexValue *value) {
    apex_error_runtime(
        "operation '%s' on '%s' not supported",
        opr,
        apex_type_get_name(value->type));
}

static void invalid_operation2(const char *opr, ApexValue *left, ApexValue *right) {
    apex_error_runtime(
        "invalid operation '%s' on '%s' and '%s'",
        opr,
        apex_type_get_name(left->type),
        apex_type_get_name(right->type));
}

ApexVm *apex_vm_new(ApexHashTable *table, const char *filename) {
    ApexVm *vm = APEX_NEW(ApexVm);

    vm->table = table;
    vm->stack = apex_malloc(STACK_INIT_SIZE * sizeof(ApexValue));
    vm->stack_top = vm->stack;
    vm->stack_size = STACK_INIT_SIZE;
    vm->stack_n = 0;
    vm->debug = 0;
    load_code(vm, filename);
    return vm;
}

void apex_vm_destroy(ApexVm *vm) {
    apex_free(vm->stack);
    apex_free(vm);
}

ApexValue *apex_vm_pop(ApexVm *vm) {
    if (vm->stack_n == 0) {
        return NULL;
    }
    vm->stack_n--;
    return --vm->stack_top;
}

static void indent(size_t count) {
    size_t i;
    count *= 4;

    for (i = 0; i < count; i++) {
        putchar(' ');
    }
}

static void print_stack(ApexVm *vm) {
    int i;
    indent(1);
    printf("STACK:\n");
    for (i = 0; i < vm->stack_n; i++) {
        ApexValue *value = vm->stack + i;
        
        indent(2);
        switch (value->type) {
        case APEX_TYPE_INT:
            printf("stack[%d] = int: %d\n", i, APEX_VALUE_INT(value));
            break;

        case APEX_TYPE_FLT:
            printf("stack[%d] = flt: %f\n", i, APEX_VALUE_FLT(value));
            break;

        case APEX_TYPE_STR:
            printf("stack[%d] = str: %s\n", i, APEX_VALUE_STR(value));
            break;

        default:
            printf("stack[%d] = ???\n", i);
            break;
        }
    }
}

static void print_op(ApexVm *vm) {
    indent(1);
    switch (*vm->p) {
    case APEX_VM_OP_PUSH_INT:
        printf("%d: %s (%d)\n", APEX_VM_OP_PUSH_INT, "push_int", *((int *)(vm->p + 1)));
        break;

    case APEX_VM_OP_ADD:
        printf("%d: %s\n", APEX_VM_OP_ADD, "add");
        break;

    case APEX_VM_OP_SUB:
        printf("%d: %s\n", APEX_VM_OP_ADD, "sub");
        break;

    case APEX_VM_OP_MUL:
        printf("%d: %s\n", APEX_VM_OP_ADD, "mul");
        break;

    case APEX_VM_OP_DIV:
        printf("%d: %s\n", APEX_VM_OP_ADD, "div");
        break;

    case APEX_VM_OP_CALL:
        printf("%d: print\n", APEX_VM_OP_CALL);
        break;

    case APEX_VM_OP_END:
        printf("%d: %s\n", APEX_VM_OP_END, "end");
        break;
    }
}

static void push_int(ApexVm *vm) {
    vm->p++;
    APEX_VALUE_INT(vm->stack_top) = *((int *)vm->p);
    vm->stack_top->type = APEX_TYPE_INT;
    vm->stack_top++;
    vm->p += sizeof(int);
    vm->stack_n++;
}

static void push_str(ApexVm *vm) {
    vm->p++;
    APEX_VALUE_STR(vm->stack_top) = ((char *)vm->p);
    vm->stack_top->type = APEX_TYPE_STR;
    vm->p += strlen(APEX_VALUE_STR(vm->stack_top)) + 1;
    vm->stack_top++;
    vm->stack_n++;
}

ApexValue *apex_vm_top(ApexVm *vm) {
    return TOP(vm, 0);
}

#define BIN_EXPR(vm, opr) do {                                  \
    ApexValue *left = apex_vm_pop(vm);                          \
    ApexValue *right = TOP(vm, 0);                              \
    switch (left->type) {                                       \
    case APEX_TYPE_INT:                                         \
        switch (right->type) {                                  \
        case APEX_TYPE_INT:                                     \
            APEX_VALUE_INT(right) opr##= APEX_VALUE_INT(left);  \
            break;                                              \
        case APEX_TYPE_FLT:                                     \
            APEX_VALUE_FLT(right) opr##= APEX_VALUE_INT(left);  \
            break;                                              \
        case APEX_TYPE_STR:                                     \
            invalid_operation2(#opr, left, right);              \
            break;                                              \
        }                                                       \
        break;                                                  \
    case APEX_TYPE_FLT:                                         \
        switch (right->type) {                                  \
        case APEX_TYPE_INT:                                     \
            APEX_VALUE_FLT(right) opr##= APEX_VALUE_INT(left);  \
            break;                                              \
        case APEX_TYPE_FLT:                                     \
            APEX_VALUE_FLT(right) opr##= APEX_VALUE_INT(left);  \
            break;                                              \
        case APEX_TYPE_STR:                                     \
            invalid_operation2(#opr, left, right);              \
            break;                                              \
        }                                                       \
        break;                                                  \
    case APEX_TYPE_STR:                                         \
        invalid_operation2(#opr, left, right);                  \
        break;                                                  \
    default:                                                    \
        apex_error_internal("invalid type: %d", left->type);    \
        break;                                                  \
    }                                                           \
    vm->p++;                                                    \
} while (0)

static void add(ApexVm *vm) {
    if (TOP(vm, 0)->type == APEX_TYPE_STR && TOP(vm, 1)->type == APEX_TYPE_STR) {

    } else {
        BIN_EXPR(vm, +);
    }
}

static void sub(ApexVm *vm) {
    BIN_EXPR(vm, -);    
}

static void mul(ApexVm *vm) {
    BIN_EXPR(vm, *);    
}

static void div(ApexVm *vm) {
    BIN_EXPR(vm, /);    
}

static void load_ref(ApexVm *vm) {
    char *id;
    size_t len;
    ApexValue *value;
    ApexType type;

    vm->p++;
    id = (char *)vm->p;
    len = strlen(id) + 1;
    vm->p += len;
    value = apex_hash_table_get_value(vm->table, id);

    if (!value) {
        apex_error_reference("undefined identifier: %s", id);
    }
    type = APEX_TYPE_OF(value);
    
    switch (type) {
    case APEX_TYPE_INT:
        APEX_VALUE_INT(vm->stack_top) = APEX_VALUE_INT(value);
        break;

    case APEX_TYPE_FUNC:
        APEX_VALUE_FUNC(vm->stack_top) = APEX_VALUE_FUNC(value);
        break;

    default: 
        break;
    }
    vm->stack_top->type = type;
    vm->stack_top++;
    vm->stack_n++;
}

static void call(ApexVm *vm) {
    ApexValue *top;
    int argc;
    ApexFunc *func;

    top = apex_vm_pop(vm);

    switch (APEX_TYPE_OF(top)) {
    case APEX_TYPE_FUNC:
        vm->p++;
        argc = *((int *)vm->p);
        vm->p += sizeof(int);
        APEX_VALUE_FUNC(top)(argc);
        break;

    default:
        apex_error_reference(
            "called object (%s) is not a function",
            apex_type_get_name(APEX_TYPE_OF(top)));
    
    }
    
}

static void import(ApexVm *vm) {
    ApexValue *top = apex_vm_pop(vm);
    apex_hash_table_import(vm->table, APEX_VALUE_STR(top));
    vm->p++;
}

void apex_vm_set_debug(ApexVm *vm, int value) {
    vm->debug = value;
}

void apex_vm_dispatch(ApexVm *vm) {
    int end = 0;

    vm->p = vm->code.bytes;
    while (!end) {
        if (vm->debug) {
            print_op(vm);
        }
        switch (*vm->p) {
        case APEX_VM_OP_PUSH_INT:
            push_int(vm);
            break;

        case APEX_VM_OP_PUSH_STR:
            push_str(vm);
            break;

        case APEX_VM_OP_ADD:
            add(vm);
            break;

        case APEX_VM_OP_SUB:
            sub(vm);
            break;

        case APEX_VM_OP_MUL:
            mul(vm);
            break;

        case APEX_VM_OP_DIV:
            div(vm);
            break;

        case APEX_VM_OP_LOAD_REF:
            load_ref(vm);
            break;

        case APEX_VM_OP_CALL:          
            call(vm);
            break;

        case APEX_VM_OP_IMPORT:
            import(vm);
            break;

        case APEX_VM_OP_END:          
            end = 1;
            break;

         default:
            apex_error_internal("invalid opcode: %d", *vm->p);
            return;
        }
        if (vm->debug) {
            print_stack(vm);
        }
    }
}

void apex_vm_print(ApexVm *vm) {
    vm->p = vm->code.bytes;
    for (;;) {
        switch (*vm->p) {
        case APEX_VM_OP_PUSH_INT:
            vm->p++;
            printf("[%d] %s (%d)\n", APEX_VM_OP_PUSH_INT, "push_int", *((int *)vm->p));
            vm->p += sizeof(int);
            break;

        case APEX_VM_OP_PUSH_STR: {
            vm->p++;
            printf("[%d] %s (%s)\n", APEX_VM_OP_PUSH_STR, "push_str", (char *)vm->p);
            vm->p += strlen((char *)vm->p) + 1;
            break;
        }

        case APEX_VM_OP_LOAD_REF: {
            vm->p++;
            printf("[%d] %s (%s)\n", APEX_VM_OP_LOAD_REF, "load_ref", (char *)vm->p);
            vm->p += strlen((char *)vm->p) + 1;
            break;
        }

        case APEX_VM_OP_ADD:
            printf("[%d] %s\n", APEX_VM_OP_ADD, "add");
            vm->p++;
            break;

        case APEX_VM_OP_SUB:
            printf("[%d] %s\n", APEX_VM_OP_ADD, "sub");
            vm->p++;
            break;

        case APEX_VM_OP_MUL:
            printf("[%d] %s\n", APEX_VM_OP_ADD, "mul");
            vm->p++;
            break;

        case APEX_VM_OP_DIV:
            printf("[%d] %s\n", APEX_VM_OP_ADD, "div");
            vm->p++;
            break;

        case APEX_VM_OP_CALL:
            vm->p++;
            printf("[%d] call (%d)\n", APEX_VM_OP_CALL, *((int *)vm->p));
            vm->p += sizeof(int);
            break;

        case APEX_VM_OP_IMPORT:
            vm->p++;
            printf("[%d] import\n", APEX_VM_OP_IMPORT);
            break;

        case APEX_VM_OP_END:
            printf("[%d] %s\n", APEX_VM_OP_END, "end");
            goto end;

        default:
            apex_error_internal("invalid opcode: %d", *vm->p);
            break;
        }
    }
end:
    return;
}
