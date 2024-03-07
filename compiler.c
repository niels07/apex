#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "lexer.h"
#include "vm.h"
#include "hash.h"

typedef struct CodeWriter {
    FILE *file;
} CodeWriter;

typedef struct Code {
    ApexVmOp opcode;
    int operand1;
    int operand2;
} Code;

int get_function_index(ApexHash_table *table, const char *name) {
    ApexHash_entry *entry = ApexHash_table_get_entry(table, name);
    if (entry != NULL) {
        return entry->index;
    } else {
        return -1; 
    }
}

int get_variable_index(ApexHash_table *table, const char *name) {
    ApexHash_entry *entry = ApexHash_table_get_entry(table, name);
    if (entry != NULL) {
        return entry->index;
    } else {
        return -1;
    }
}

static CodeWriter *CodeWriter_new(FILE *file) {
    CodeWriter *writer = (CodeWriter *)malloc(sizeof(CodeWriter));
    if (writer) {
        writer->file = file;
    }
    return writer;
}

static void CodeWriter_write_instruction(CodeWriter *writer, Code *instr) {
    if (!writer || !writer->file || !instr) {
        return;
    }

    fwrite(instr, sizeof(Code), 1, writer->file);
}

static void CodeWriter_free(CodeWriter *writer) {
    if (writer) {
        fclose(writer->file);
        free(writer);
    }
}

void ApexCompiler_write(ApexLexer *lexer, FILE *file, ApexHash_table *table) {
    CodeWriter *writer;
    Code instr;

    if (!lexer || !file) {
        ApexError_throw(
            APEX_ERROR_CODE_COMPILER,
            "Lexer or file is NULL.");
        return;
    }

    writer = CodeWriter_new(file);
    if (!writer) {
        ApexError_throw(
            APEX_ERROR_CODE_MEMORY,
            "Failed to allocate memory for CodeWriter.");
        fclose(file);
        return;
    }

    while (ApexLexer_next_token(lexer) != APEX_LEXER_TOKEN_EOF) {
        switch (ApexLexer_next_token(lexer)) {
            case APEX_LEXER_TOKEN_INT:
                instr.opcode = APEX_VM_OP_PUSH_INT;
                instr.operand1 = ApexLexer_get_int(lexer);
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_FLT:
                instr.opcode = APEX_VM_OP_PUSH_FLT; 
                instr.operand1 = ApexLexer_get_int(lexer);
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_STR:
                instr.opcode = APEX_VM_OP_PUSH_STR;
                instr.operand1 = 0;
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_ID:
                if (ApexLexer_peek(lexer) == '(') {
                    instr.opcode = APEX_VM_OP_CALL; 
                    instr.operand1 = get_function_index(table, ApexLexer_get_str(lexer));
                    instr.operand2 = 0;
                    ApexLexer_next_token(lexer);
                } else {
                    instr.opcode = APEX_VM_OP_LOAD_REF; 
                    instr.operand1 =get_variable_index(table, ApexLexer_get_str(lexer));
                    instr.operand2 = 0;
                }
                break;

            case APEX_LEXER_TOKEN_EQUALS:
                instr.opcode = APEX_VM_OP_EQUALS;
                instr.operand1 = 0;
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_IF:
                instr.opcode = APEX_VM_OP_IF; 
                instr.operand1 = 0;
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_ELSE:
                instr.opcode = APEX_VM_OP_ELSE;
                instr.operand1 = 0;
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_WHILE:
                instr.opcode = APEX_VM_OP_WHILE; 
                instr.operand1 = 0;
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_DEF:
                instr.opcode = APEX_VM_OP_FUNC_DEF; 
                instr.operand1 = get_function_index(table, ApexLexer_get_str(lexer));
                instr.operand2 = 0;
                break;

            case APEX_LEXER_TOKEN_END_STMT:
                if (ApexLexer_peek(lexer) == '(') {
                    instr.opcode = APEX_VM_OP_CALL;
                    instr.operand1 = get_function_index(table, ApexLexer_get_str(lexer));
                    instr.operand2 = 0;
                }
                break;

            case APEX_LEXER_TOKEN_NOT:
                instr.opcode = APEX_VM_OP_NOT;
                instr.operand1 = 0;
                instr.operand2 = 0;
                break;

            default:
                ApexError_throw(
                    APEX_ERROR_CODE_COMPILER,
                    "Unexpected token encountered during compilation.");
                CodeWriter_free(writer);
                return;
        }

        CodeWriter_write_instruction(writer, &instr);
    }

    fclose(file);
    CodeWriter_free(writer);
}

void ApexCompiler_read(ApexVm *vm, const char *filename) {
  
}
