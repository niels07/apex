#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "value.h"
#include "hash.h"
#include "vm.h"
#include "error.h"
#include "malloc.h"

#define STACK_INIT_SIZE 256
#define STACK_SIZE 100

#define TOP(vm, i) ((vm)->stack_top - i - 1)
#define TYPE_OF(value) value->type

static void load_code(ApexVm *vm, const char *bin_file_name) {
    long file_size;
    FILE *file = fopen(bin_file_name, "rb");
    if (!file) {
        ApexError_io("could not open file: %s", strerror(errno));
        return;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    vm->code.bytes = apex_malloc(file_size + 1);
    

    if (fread(vm->code.bytes, file_size, 1, file) != 1) {
        ApexError_io("read failed: %s", strerror(errno));
        fclose(file);
        apex_free(vm->code.bytes);
        vm->code.bytes = NULL;
        return;
    }

    vm->code.bytes[file_size] = '\0'; 
    vm->code.size = file_size;
    fclose(file);
}

ApexVm *ApexVm_new(ApexHash_table *table, const char *bin_file_name) {
    ApexVm *vm = apex_malloc(sizeof(ApexVm));

    vm->table = table;
    vm->stack = apex_malloc(STACK_INIT_SIZE * sizeof(ApexValue));
    vm->stack_top = vm->stack;
    vm->stack_size = STACK_INIT_SIZE;
    vm->stack_n = 0;
    load_code(vm, bin_file_name);
    return vm;
}

void ApexVm_destroy(ApexVm *vm) {
    apex_free(vm->stack);
    apex_free(vm);
}

ApexValue* ApexVm_pop(ApexVm *vm) {
    if (vm->stack_top > vm->stack) {
        return --vm->stack_top;
    } else {
        ApexError_runtime("Stack underflow");
        return NULL;
    }
}

ApexValue* ApexVm_top(ApexVm *vm) {
    if (vm->stack_top > vm->stack) {
        return vm->stack_top - 1;
    } else {
        ApexError_runtime("Stack is empty");
        return NULL;
    }
}

static void push_int(ApexVm *vm) {
    ApexValue value;
    int operand_size = sizeof(int);
    int i;

    if (vm->stack_top - vm->stack >= STACK_SIZE) {
        ApexError_runtime("Stack Overflow");
        return;
    }

    if (!vm->p) {
        ApexError_runtime("Code pointer is empty");
        return;
    }
 
    if (*vm->p + 1 > vm->code.size) {
        ApexError_runtime("Insufficient bytes for operand size");
        return;
    }

    value.type = APEX_VALUE_TYPE_INT;
    for (i = 0; i < operand_size; i++) {
        value.data.int_val |= (vm->code.bytes[*vm->p + i]) << (8 * i);
    }

    *vm->stack_top++ = value;
    *vm->p += operand_size; 
}

static void push_str(ApexVm *vm) {
    ApexValue value;
    int len;
    char *str;
    int i;

    if (vm->stack_top - vm->stack >= STACK_SIZE) {
        ApexError_runtime("Stack Overflow");
        return;
    }

    (*vm->p)++;

    if (vm->code.n <= *vm->p) {
        ApexError_runtime("Invalid program counter position");
        return;
    }

    len = (int)vm->code.bytes[*vm->p];
    (*vm->p)++;

    if (vm->code.n <= *vm->p + len - 1) {
        ApexError_runtime("Invalid string length");
        return;
    }

    str = apex_malloc((len + 1) * sizeof(char));
    for (i = 0; i < len; ++i) {
        str[i] = vm->code.bytes[*vm->p + i];
    }
    str[len] = '\0';

    value.type = APEX_VALUE_TYPE_STR;
    value.data.str_val = str;

    *vm->stack_top++ = value;
    (*vm->p) += len;
}

#define BIN_EXPR(vm, opr) do {                                       \
    ApexValue *left = ApexVm_pop(vm);                                \
    ApexValue *right = TOP(vm, 0);                                   \
    switch (left->type) {                                            \
    case APEX_VALUE_TYPE_INT:                                        \
        switch (right->type) {                                       \
        case APEX_VALUE_TYPE_INT:                                    \
            right->data.int_val opr##= left->data.int_val;           \
            break;                                                   \
        case APEX_VALUE_TYPE_FLT:                                    \
            right->data.flt_val opr##= left->data.int_val;           \
            break;                                                   \
        case APEX_VALUE_TYPE_STR:                                    \
            invalid_operation(#opr, left, right);                   \
            break;                                                   \
        case APEX_VALUE_TYPE_BOOL:                                   \
            if (#opr[0] == '!') right->data.bool_val = !left->data.int_val; \
            else right->data.bool_val opr##= left->data.int_val;     \
            break;                                                   \
        }                                                            \
        break;                                                       \
    case APEX_VALUE_TYPE_FLT:                                        \
        switch (right->type) {                                       \
        case APEX_VALUE_TYPE_INT:                                    \
            right->data.flt_val opr##= left->data.int_val;           \
            break;                                                   \
        case APEX_VALUE_TYPE_FLT:                                    \
            right->data.flt_val opr##= left->data.int_val;           \
            break;                                                   \
        case APEX_VALUE_TYPE_STR:                                    \
            invalid_operation(#opr, left, right);                   \
            break;                                                   \
        case APEX_VALUE_TYPE_BOOL:                                   \
            if (#opr[0] == '!') right->data.bool_val = !left->data.int_val; \
            else right->data.bool_val opr##= left->data.int_val;     \
            break;                                                   \
        }                                                            \
        break;                                                       \
    case APEX_VALUE_TYPE_STR:                                        \
        invalid_operation(#opr, left, right);                       \
        break;                                                       \
    case APEX_VALUE_TYPE_BOOL:                                       \
        switch (right->type) {                                       \
        case APEX_VALUE_TYPE_INT:                                    \
            right->data.int_val opr##= left->data.bool_val;          \
            break;                                                   \
        case APEX_VALUE_TYPE_FLT:                                    \
            right->data.flt_val opr##= left->data.bool_val;          \
            break;                                                   \
        case APEX_VALUE_TYPE_STR:                                    \
            invalid_operation(#opr, left, right);                   \
            break;                                                   \
        case APEX_VALUE_TYPE_BOOL:                                   \
            if (#opr[0] == '!') right->data.bool_val = !left->data.bool_val; \
            else right->data.bool_val opr##= left->data.bool_val;    \
            break;                                                   \
        }                                                            \
        break;                                                       \
    default:                                                         \
        ApexError_internal("invalid type: %d", left->type);          \
        break;                                                       \
    }                                                                \
    vm->p++;                                                         \
} while (0)

static void invalid_operation(const char *opr, ApexValue *left, ApexValue *right) {
    ApexError_runtime(
        "invalid operation '%s' on '%s' and '%s'",
        opr,
        ApexValue_get_type_name(left->type),
        ApexValue_get_type_name(right->type));
}

static void add(struct ApexVm *vm) {
    if (TOP(vm, 0)->type == APEX_VALUE_TYPE_STR && TOP(vm, 1)->type == APEX_VALUE_TYPE_STR) {

    } else {
        BIN_EXPR(vm, +);
    }
}

static void sub(struct ApexVm *vm) {
    BIN_EXPR(vm, -);    
}

static void mul(struct ApexVm *vm) {
    BIN_EXPR(vm, *);    
}

static void div(struct ApexVm *vm) {
    BIN_EXPR(vm, /);    
}

static void load_ref(struct ApexVm *vm) {
    char *id;
    size_t len;
    ApexValue *value;

    vm->p++;
    id = (char *)vm->p;
    len = strlen(id) + 1;
    vm->p += len;
    
    value = ApexHash_table_get_value(vm->table, id);
    if (value == NULL) {
        ApexError_reference("undefined identifier: %s", id);
        return;
    }

    *vm->stack_top++ = *value;
    vm->stack_n++;
}

static void call(struct ApexVm *vm) {
    ApexValue *top;
    int argc;

    top = ApexVm_pop(vm);

    switch (TYPE_OF(top)) {
    case APEX_VALUE_TYPE_FUNC:
        vm->p++;
        argc = *((int *)vm->p);
        vm->p += sizeof(int);
        ((ApexValue_function)(top->data.func_val))(vm, argc);
        break;

    default:
        ApexError_reference(
            "called object (%s) is not a function",
            ApexValue_get_type_name(TYPE_OF(top)));
    
    }
}

static void func_def(struct ApexVm *vm) {
    ApexValue *top;
    char *id;
    size_t len;

    top = ApexVm_pop(vm);

    switch (TYPE_OF(top)) {
    case APEX_VALUE_TYPE_FUNC:
        vm->p++;
        id = (char *)vm->p;
        len = strlen(id) + 1;
        vm->p += len;
        ApexHash_table_insert_func(vm->table, id, top->data.func_val);
        break;

    default:
        ApexError_reference(
            "function definition requires a function pointer, got %s",
            ApexValue_get_type_name(TYPE_OF(top)));
    
    }
}

static void if_else(struct ApexVm *vm) {
    ApexValue *condition = ApexVm_pop(vm);
    int is_true = 0;
    int else_found = 0;

    switch (TYPE_OF(condition)) {
    case APEX_VALUE_TYPE_BOOL:
        is_true = condition->data.bool_val;
        break;

    default:
        ApexError_reference(
            "if condition must be a boolean, got %s",
            ApexValue_get_type_name(TYPE_OF(condition)));
    }

    if (is_true) {
        while (*vm->p != APEX_VM_OP_ELSE && *vm->p != APEX_VM_OP_END) {
            ApexVm_dispatch(vm);
        }
    } else {
        while (*vm->p != APEX_VM_OP_ELSE && *vm->p != APEX_VM_OP_END) {
            vm->p++;
        }
        if (*vm->p == APEX_VM_OP_ELSE) {
            vm->p++;
            else_found = 1;
        }
    }

    if (else_found || !is_true) {
        while (*vm->p != APEX_VM_OP_END) {
            ApexVm_dispatch(vm);
        }
    }

    vm->p++;
}

static void while_loop(struct ApexVm *vm) {
    ApexValue *condition;
    int is_true = 0;

    while (1) {
        condition = ApexVm_pop(vm);

        switch (TYPE_OF(condition)) {
        case APEX_VALUE_TYPE_BOOL:
            is_true = condition->data.bool_val;
            break;

        default:
            ApexError_reference(
                "while loop condition must be a boolean, got %s",
                ApexValue_get_type_name(TYPE_OF(condition)));
        }

        if (!is_true) {
            break;
        }

        while (*vm->p != APEX_VM_OP_END) {
            ApexVm_dispatch(vm);
        }

        vm->p -= 3;
    }

    vm->p++;
}

void ApexVm_dispatch(ApexVm *vm) {
    int end = 0;

    vm->p = vm->code.bytes;
    while (!end) {
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

        case APEX_VM_OP_FUNC_DEF:
            func_def(vm);
            break;

        case APEX_VM_OP_IF:
            if_else(vm);
            break;

        case APEX_VM_OP_WHILE:
            while_loop(vm);
            break;

        case APEX_VM_OP_END:          
            end = 1;
            break;

         default:
            ApexError_internal("invalid opcode: %d", *vm->p);
            return;
        }
    }
}