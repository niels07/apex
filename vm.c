#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "symbol.h"
#include "mem.h"
#include "bool.h"
#include "string.h"
#include "error.h"
#include "stdlib.h"

#define STACK_PUSH(vm, val) ((vm)->stack[(vm)->stack_top++] = (val))
#define STACK_POP(vm)       ((vm)->stack[--(vm)->stack_top])

static const char *opcode_to_string(OpCode opcode) {
    switch (opcode) {
        case OP_PUSH_INT: return "OP_PUSH_INT";
        case OP_PUSH_FLT: return "OP_PUSH_FLT";
        case OP_PUSH_STR: return "OP_PUSH_STR";
        case OP_PUSH_BOOL: return "OP_PUSH_BOOL";
        case OP_POP: return "OP_POP";
        case OP_ADD: return "OP_ADD";
        case OP_SUB: return "OP_SUB";
        case OP_MUL: return "OP_MUL";
        case OP_DIV: return "OP_DIV";
        case OP_RETURN: return "OP_RETURN";
        case OP_CALL: return "OP_CALL";
        case OP_JUMP: return "OP_JUMP";
        case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
        case OP_SET_GLOBAL: return "OP_SET_GLOBAL";
        case OP_GET_GLOBAL: return "OP_GET_GLOBAL";
        case OP_GET_LOCAL: return "OP_GET_LOCAL";    
        case OP_SET_LOCAL: return "OP_SET_LOCAL";
        case OP_NOT: return "OP_NOT";
        case OP_NEGATE: return "OP_NEGATE";
        case OP_CALL_LIB: return "OP_CALL_LIB";
        case OP_FUNCTION_START: return "OP_FUNCTION_START";
        case OP_FUNCTION_END: return "OP_FUNCTION_END";
        case OP_EQ: return "OP_EQ";
        case OP_NE: return "OP_NE";
        case OP_LT: return "OP_LT";
        case OP_LE: return "OP_LE";
        case OP_GT: return "OP_GT";
        case OP_GE: return "OP_GE";
        case OP_HALT: return "OP_HALT";
        default: return "UNKNOWN_OPCODE";
    }
}

void print_vm_instructions(ApexVM *vm) {
    int i;
    printf("== ApexVM Instructions ==\n");
    for (i = 0; i < vm->chunk->ins_n; i++) {
        Ins *instr = &vm->chunk->ins[i];
        printf("%04d: %-20s", i, opcode_to_string(instr->opcode));
        
        switch (instr->opcode) {
        case OP_PUSH_INT:
            printf("%d", instr->value.intval);
            break;

        case OP_PUSH_FLT:
            printf("%f", instr->value.fltval);
            break;

        case OP_PUSH_STR:
            printf("\"%s\"", instr->value.strval);
            break;

        case OP_PUSH_BOOL:
            printf("%s", instr->value.boolval ? "true" : "false");
            break;
        
        case OP_CALL:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP:
        case OP_SET_LOCAL:
            printf("%d", instr->value.intval);
            break;

        case OP_SET_GLOBAL:
        case OP_GET_GLOBAL:
            printf("\"%s\"", instr->value.strval);
            break;

        case OP_GET_LOCAL:
            printf("%d", instr->value.intval);
            break;

        default:
            break;
        }
        
        printf("\n");
    }
}

void init_vm(ApexVM *vm) {
    vm->stack_top = 0;
    vm->ip = 0;
    vm->in_function = FALSE;
    vm->chunk = mem_alloc(sizeof(Chunk));
    vm->chunk->ins = mem_alloc(sizeof(Ins) * 8);
    vm->chunk->ins_size = 8;
    vm->chunk->ins_n = 0;
    vm->loop_start = -1;
    vm->loop_end = -1;
    vm->lineno = 0;
    init_symbol_table(&vm->global_table);
    init_symbol_table(&vm->fn_table);
    init_scope_stack(&vm->local_scopes);
}

void free_vm(ApexVM *vm) {
    free(vm->chunk->ins);
    free(vm->chunk);
    free_symbol_table(&vm->global_table);
    free_symbol_table(&vm->fn_table);
    free_scope_stack(&vm->local_scopes);
}

static void stack_push(ApexVM *vm, ApexValue value) {
    if (vm->stack_top >= STACK_MAX) {
        fprintf(stderr, "Stack overflow\n");
        exit(EXIT_FAILURE);
    }
    vm->stack[vm->stack_top++] = value;
}

static ApexValue stack_pop(ApexVM *vm) {
    if (vm->stack_top == 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(EXIT_FAILURE);
    }
    return vm->stack[--vm->stack_top];
}

void apexVM_pushval(ApexVM *vm, ApexValue value) {
    stack_push(vm, value);
}

void apexVM_pushstr(ApexVM *vm, const char *str) {
    char *s = new_string(str, strlen(str));
    stack_push(vm, apexVal_makestr(s));
}

void apexVM_pushint(ApexVM *vm, int i) {
    stack_push(vm, apexVal_makeint(i));
}

void apexVM_pushflt(ApexVM *vm, float flt) {
    stack_push(vm, apexVal_makeflt(flt));
}

void apexVM_pushbool(ApexVM *vm, Bool b) {
    stack_push(vm, apexVal_makebool(b));
}

ApexValue apexVM_pop(ApexVM *vm) {
    return stack_pop(vm);
}

static ApexValue vm_add(int lineno, ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        return apexVal_makeint(a.intval + b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        ApexValue v = apexVal_makeflt(a.fltval + b.fltval);
        return v;
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval + b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval + b.intval);
    }
    if (a.type == APEX_VAL_STR && b.type == APEX_VAL_STR) {
        char *newstr = cat_string(b.strval, a.strval);
        return apexVal_makestr(newstr);
    }
    if (a.type == APEX_VAL_BOOL && b.type == APEX_VAL_BOOL) {
        apexErr_runtime(lineno, "cannot perform arithmetic on a boolean value");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_INT) || 
        (a.type == APEX_VAL_INT && b.type == APEX_VAL_STR)) {
        apexErr_runtime(lineno, "cannot add string to an int");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_FLT) || 
        (a.type == APEX_VAL_FLT && b.type == APEX_VAL_STR)) {
        apexErr_runtime(lineno, "cannot add string to an flt");
    }
    if ((a.type == APEX_VAL_INT && b.type == APEX_VAL_BOOL) || 
        (a.type == APEX_VAL_BOOL && b.type == APEX_VAL_INT)) {
        apexErr_runtime(lineno, "cannot add bool to an int");
    }
    if ((a.type == APEX_VAL_STR && b.type == APEX_VAL_BOOL) || 
        (a.type == APEX_VAL_BOOL && b.type == APEX_VAL_STR)) {
        apexErr_runtime(lineno, "cannot add string to a bool");
    }
    return apexVal_makenull();
}

static ApexValue vm_sub(int lineno,ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        return apexVal_makeint(a.intval - b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.fltval - b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval - b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval - b.intval);
    }
    apexErr_runtime(
        lineno, 
        "cannot subtract %s from %s", 
        get_value_type_str(a.type), 
        get_value_type_str(b.type));
    return apexVal_makenull();
}

static ApexValue vm_mul(int lineno,ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        return apexVal_makeint(a.intval * b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        ApexValue v = apexVal_makeflt(a.fltval * b.fltval);
        return v;
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        return apexVal_makeflt(a.intval * b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        return apexVal_makeflt(a.fltval * b.intval);
    }
    apexErr_runtime(
        lineno, 
        "cannot multiply %s with %s", 
        get_value_type_str(a.type), 
        get_value_type_str(b.type));
    return apexVal_makenull();
}

#define CHECK_DIV_ZERO(x) do { \
    if (x == 0) { \
        apexErr_runtime(lineno, "division by zero"); \
    } \
} while (0)

static ApexValue vm_div(int lineno,ApexValue a, ApexValue b) {
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        CHECK_DIV_ZERO(b.intval);
        return apexVal_makeint(a.intval / b.intval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(b.fltval);
        return apexVal_makeflt(a.fltval / b.fltval);
    }
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        CHECK_DIV_ZERO(b.fltval);
        return apexVal_makeflt(a.intval / b.fltval);
    }
    if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        CHECK_DIV_ZERO(b.intval);
        return apexVal_makeflt(a.fltval / b.intval);
    }
    apexErr_runtime(
        lineno, 
        "cannot divide %s by %s", 
        get_value_type_str(a.type), 
        get_value_type_str(b.type));
    return apexVal_makenull();
}

static ApexValue cmp(ApexValue a, ApexValue b, OpCode opcode) {
#define CMP_NUMS() do { \
    switch (opcode) { \
    case OP_EQ: result = left == right; break; \
    case OP_NE: result = left != right; break; \
    case OP_LT: result = left < right; break; \
    case OP_LE: result = left <= right; break; \
    case OP_GT: result = left > right; break; \
    case OP_GE: result = left >= right; break; \
    default: result = FALSE; break; \
    } \
} while (0)
    Bool result = FALSE;
    if (a.type == APEX_VAL_INT && b.type == APEX_VAL_INT) {
        int left = a.intval;
        int right = b.intval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_FLT) {
        float left = a.fltval;
        float right = b.fltval;
        CMP_NUMS();
    } else if (a.type == APEX_VAL_INT && b.type == APEX_VAL_FLT) {
        float left = a.intval;
        float right = b.fltval;
       CMP_NUMS();
    } else if (a.type == APEX_VAL_FLT && b.type == APEX_VAL_INT) {
        float left = a.fltval;
        float right = b.intval;
        CMP_NUMS();
    } else if (a.type != b.type) {
        result = FALSE;
    } else if (a.type == APEX_VAL_BOOL) {
        if (opcode == OP_EQ) {
            result = a.boolval == b.boolval;
        } else if (opcode == OP_NE) {
            result = a.boolval != b.boolval;
        } else {
            result = FALSE;
        }        
    } else if (a.type == APEX_VAL_STR) {
        if (opcode == OP_EQ) {
            result = a.strval == b.strval;
        } else if (opcode == OP_NE) {
            result = a.strval != b.strval;
        } else {
            result = FALSE;
        }
    } else if (a.type == APEX_VAL_NULL) {
        if (opcode == OP_EQ) {
            result = TRUE;
        } else if (opcode == OP_NE) {
            result = FALSE;
        }
    } else if (a.type == APEX_VAL_FN) {
        if (opcode == OP_EQ) {
            result = a.fnval == b.fnval;
        } else if (opcode == OP_NE) {
            result = a.fnval != b.fnval;
        }
    }
    return apexVal_makebool(result);
}

void vm_dispatch(ApexVM *vm) {
    for (;;) {
        Ins *ins = &vm->chunk->ins[vm->ip++];

        switch (ins->opcode) {
        case OP_PUSH_INT:
            stack_push(vm, ins->value);
            break;

        case OP_PUSH_FLT:
            stack_push(vm, ins->value);
            break;

        case OP_PUSH_STR:
            stack_push(vm, ins->value);
            break;

        case OP_PUSH_BOOL:
            stack_push(vm, ins->value);
            break;

        case OP_POP:
            stack_pop(vm);
            break;

        case OP_ADD: {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_add(vm->lineno, a, b));
            break;
        }
        case OP_SUB: {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_sub(vm->lineno, a, b));
            break;
        }

        case OP_MUL: {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_mul(vm->lineno, a, b));
            break;
        }
        case OP_DIV:  {
            ApexValue a = stack_pop(vm);
            ApexValue b = stack_pop(vm);
            stack_push(vm, vm_div(vm->lineno, a, b));
            break;
        }

        case OP_RETURN: {
            ApexValue ret_val;
            int ret_addr;

            if (vm->stack_top > 0) {
                ret_val = stack_pop(vm);
            } else {
                ret_val = apexVal_makenull();
            }

            ret_addr = stack_pop(vm).intval;
            vm->ip = ret_addr;
            stack_push(vm, ret_val);
            pop_scope(&vm->local_scopes);
            break;
        }

        case OP_CALL: {
            int fn_addr = ins->value.intval;
            int ret_addr = vm->ip;
            Fn *fn;
            int i;

            fn = get_symbol_value_by_addr(&vm->fn_table, fn_addr).fnval;
            push_scope(&vm->local_scopes);

            for (i = fn->param_n - 1; i >= 0; i--) {
                ApexValue arg = stack_pop(vm);
                const char *name = fn->params[i];
                SymbolAddr addr = add_local_symbol(&vm->local_scopes, name);
                set_local_symbol_value(&vm->local_scopes, addr, arg);
            }

            stack_push(vm, apexVal_makeint(ret_addr));
            vm->ip = fn->addr;
            break;
        }
        case OP_JUMP:
            vm->ip += ins->value.intval;
            break;

        case OP_JUMP_IF_FALSE: {
            ApexValue condition = stack_pop(vm);
            switch (condition.type) {
            case APEX_VAL_INT:
                condition = apexVal_makebool(condition.intval != 0);
                break;

            case APEX_VAL_FLT:
                condition = apexVal_makebool(condition.fltval != 0);
                break;

            case APEX_VAL_BOOL:
                break;

            case APEX_VAL_STR:
                condition = apexVal_makebool(condition.strval != NULL);
                break;

            case APEX_VAL_FN:
                condition = apexVal_makebool(TRUE);
                break;

            case APEX_VAL_NULL:
                condition = apexVal_makebool(FALSE);
                break;
            }

            if (!condition.boolval) {
                vm->ip += ins->value.intval;
            }
            break;
        }
        case OP_SET_GLOBAL: {
            const char *name = ins->value.strval;
            ApexValue value = stack_pop(vm);
            if (!set_symbol_value(&vm->global_table, name, value)) {
                apexErr_fatal("attemt to set an undefined global variable: '%s'", name);
            }
            break;
        }
        case OP_GET_GLOBAL: {
            const char *name = ins->value.strval;
            ApexValue value = get_symbol_value_by_name(&vm->global_table, name);
            stack_push(vm, value);
            break;
        }
        case OP_SET_LOCAL: {
            int addr = ins->value.intval;
            ApexValue value = stack_pop(vm);
            if (set_local_symbol_value(&vm->local_scopes, addr, value)) {
                apexErr_fatal("attempt to set an undefined local variable: %d", addr);
            }
            break;
        }
        case OP_GET_LOCAL: {
            int addr = ins->value.intval;
            ApexValue value = get_local_symbol_value(&vm->local_scopes, addr);
            stack_push(vm, value);
            break;
        }
        case OP_NOT:
            stack_push(vm, apexVal_makebool(!stack_pop(vm).boolval));
            break;

        case OP_NEGATE: {
            ApexValue val = stack_pop(vm);

            if (val.type == APEX_VAL_INT) {
                stack_push(vm, apexVal_makeint(-val.intval));
            } else if (val.type == APEX_VAL_FLT) {
                stack_push(vm, apexVal_makeflt(-val.fltval));
            } else {
                apexErr_runtime(ins->lineno, "cannot negate %s", get_value_type_str(val.type));
            }
            break;
        }

        case OP_CALL_LIB: {
            int fn_addr = ins->value.intval;
            apex_stdlib[fn_addr].fn(vm);
            break;
        }

        case OP_FUNCTION_START:
             while (vm->chunk->ins[vm->ip].opcode != OP_FUNCTION_END) {
                vm->ip++;
            }
            vm->ip++;
            break; 

        case OP_FUNCTION_END:            
            break;

        case OP_EQ:
        case OP_NE:
        case OP_LT:
        case OP_LE:
        case OP_GT:
        case OP_GE: {
            ApexValue value = cmp(stack_pop(vm), stack_pop(vm), ins->opcode);
            stack_push(vm, value);
            break;
        }
        case OP_HALT:
            return;
            
        default:
            fprintf(stderr, "Unknown opcode: %d\n", ins->opcode);
            exit(1);
        }
    }
}