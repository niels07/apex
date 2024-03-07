#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"

int main(int argc, char *argv[]) {
    FILE *input_file;
    ApexLexer_token token;
    ApexLexer *lexer;
    char *token_value;
    char *token_type_str;
    char token_value_tmp[64];
    int lineno;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    input_file = fopen(argv[1], "r");
    if (!input_file) {
        perror("Error opening input file");
        return 1;
    }

    lexer = ApexLexer_new(input_file);

    printf("%-20s%-20s%s\n", "Token", "Value", "Lineno");
    printf("=======================================================\n");
    while ((token = ApexLexer_next_token(lexer)) != APEX_LEXER_TOKEN_EOF) {
        lineno = lexer->lineno;
        switch (token) {
        case APEX_LEXER_TOKEN_INT:
            token_type_str = "INTEGER";
            sprintf(token_value_tmp, "%d", ApexLexer_get_int(lexer));
            token_value = token_value_tmp;
            break;

        case APEX_LEXER_TOKEN_FLT:
            token_type_str = "FLOAT";
            sprintf(token_value_tmp, "%f", ApexLexer_get_flt(lexer));
            token_value = token_value_tmp;
            break;

        case APEX_LEXER_TOKEN_STR:
            token_type_str = "STRING";
            token_value = ApexLexer_get_str(lexer);
            break;

        case APEX_LEXER_TOKEN_BOOL:
            token_type_str = "BOOLEAN";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_TRUE:
            token_type_str = "TRUE";
            token_value = "true";
            break;

        case APEX_LEXER_TOKEN_FALSE:
            token_type_str = "FALSE";
            token_value = "false";
            break;

        case APEX_LEXER_TOKEN_NOT:
            token_type_str = "NOT";
            token_value = "!";
            break;
        
        case APEX_LEXER_TOKEN_ID:
            token_type_str = "IDENTIFIER";
            token_value = ApexLexer_get_str(lexer);
            break;

        case APEX_LEXER_TOKEN_INC:
            token_type_str = "INCREMENT";
            token_value = "++";
            break;

        case APEX_LEXER_TOKEN_DEC:
            token_type_str = "DECREMENT";
            token_value = "--";
            break;

        case APEX_LEXER_TOKEN_ASGN_ADD:
            token_type_str = "ASSIGN_ADD";
            token_value = "+=";
            break;

        case APEX_LEXER_TOKEN_ASGN_SUB:
            token_type_str = "ASSIGN_SUBTRACT";
            token_value = "-=";
            break;

        case APEX_LEXER_TOKEN_ASGN_MUL:
            token_type_str = "ASSIGN_MULTIPLY";
            token_value = "*=";
            break;

        case APEX_LEXER_TOKEN_ASGN_DIV:
            token_type_str = "ASSIGN_DIVIDE";
            token_value = "/=";
            break;

        case APEX_LEXER_TOKEN_EQUALS:
            token_type_str = "EQUALS";
            token_value = "==";
            break;

        case APEX_LEXER_TOKEN_NOT_EQUAL:
            token_type_str = "NOT_EQUAL";
            token_value = "!=";
            break;

        case APEX_LEXER_TOKEN_LESS:
            token_type_str = "LESS_THAN";
            token_value = "<";
            break;

        case APEX_LEXER_TOKEN_LESS_EQUAL:
            token_type_str = "LESS_THAN_OR_EQUAL";
            token_value = "<=";
            break;

        case APEX_LEXER_TOKEN_GREATER:
            token_type_str = "GREATER_THAN";
            token_value = ">";
            break;

        case APEX_LEXER_TOKEN_GREATER_EQUAL:
            token_type_str = "GREATER_THAN_OR_EQUAL";
            token_value = ">=";
            break;

        case APEX_LEXER_TOKEN_ADD:
            token_type_str = "ADD";
            token_value = "+";
            break;

        case APEX_LEXER_TOKEN_DEF:
            token_type_str = "DEFINE";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_SUB:
            token_type_str = "SUBTRACT";
            token_value = "-";
            break; 

        case APEX_LEXER_TOKEN_MUL:
            token_type_str = "MULTIPLY";
            token_value = "*";
            break; 

        case APEX_LEXER_TOKEN_DIV:
            token_type_str = "DIVIDE";
            token_value = "/";
            break;

        case APEX_LEXER_TOKEN_AND:
            token_type_str = "LOGICAL_AND";
            token_value = "&&";
            break;

        case APEX_LEXER_TOKEN_OR:
            token_type_str = "LOGICAL_OR";
            token_value = "||";
            break;

        case APEX_LEXER_TOKEN_IF:
            token_type_str = "IF";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_ELSE:
            token_type_str = "ELSE";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_ELIF:
            token_type_str = "ELIF";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_WHILE:
            token_type_str = "WHILE";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_DECL_INT:
            token_type_str = "DECLARE_INTEGER";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_DECL_FLT:
            token_type_str = "DECLARE_FLOAT";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_DECL_BOOL:
            token_type_str = "DECLARE_BOOLEAN";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_DECL_STR:
            token_type_str = "DECLARE_STRING";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_DECL_FUNC:
            token_type_str = "DECLARE_FUNCTION";
            token_value = "(none)";
            break;
        
        case APEX_LEXER_TOKEN_IMPORT:
            token_type_str = "IMPORT";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_EOF:
            token_type_str = "END_OF_FILE";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_END_STMT:
            token_type_str = "END_OF_STATEMENT";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_NULL_TERM:
            token_type_str = "NULL_TERMINATOR";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_VOID:
            token_type_str = "VOID";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_RETURN:
            token_type_str = "RETURN";
            token_value = "(none)";
            break;

        case APEX_LEXER_TOKEN_END:
            token_type_str = "END_MARKER";
            token_value = "(none)";
            break;
        
        case '+':
            token_type_str = "ADD";
            token_value = "(none)";
            break;
    
        case '-':
            token_type_str = "SUBTRACT";
            token_value = "(none)";
            break;

        case '*':
            token_type_str = "MULTIPLY";
            token_value = "(none)";
            break;

         case '/':
            token_type_str = "DIVISION";
            token_value = "(none)";
            break;

        case '=':
            token_type_str = "ASSIGN";
            token_value = "(none)";
            break;

        case '(':
            token_type_str = "PAREN_OPEN";
            token_value = "(none)";
            break;
        
        case ')':
            token_type_str = "PAREN_CLOSE";
            token_value = "(none)";
            break;

        default:
            token_type_str = "UNKNOWN";
            token_value = "(none)";
            break;
        }
        printf("%-20s%-20s%d\n", token_type_str, token_value, lineno);
    }

    fclose(input_file);
    ApexLexer_free(lexer);
    return 0;
}