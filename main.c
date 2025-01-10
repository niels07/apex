#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "apexStr.h"
#include "mem.h"
#include "error.h"
#include "parser.h"
#include "vm.h"
#include "value.h"
#include "compiler.h"
#include "lib.h"

#define INPUT_BUFFER_SIZE 1024

static void print_usage(void) {
    printf("Usage: apex [file]\n");
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "failed to open file: %s\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *buffer = apexMem_alloc(size + 1);
    fread(buffer, 1, size, file);
    fclose(file);
    buffer[size] = '\0';
    
    return buffer;
}

void start_repl(void) {
    char input[INPUT_BUFFER_SIZE] = {0};
    Parser parser;
    Lexer lexer;
    init_string_table();
    apexLib_init();
    init_lexer(&lexer, NULL, NULL);
    lexer.parsestate.exit_on_error = false;

    ApexVM vm;
    init_vm(&vm);
    int lexer_pos = 0;
    bool retain_lexer_pos = false;

    printf("> ");
    while (fgets(input, sizeof(input), stdin)) {
        apexLex_feedline(&lexer, input);
        
        printf("%s\n", lexer.source);
        if (!retain_lexer_pos) {
            lexer_pos = lexer.position;            
            
        } else {
            lexer.position = lexer_pos;
        }
        init_parser(&parser, &lexer);
        parser.allow_incomplete = true;

        AST *program = parse_program(&parser);
        print_ast(program, 0);

        if (program) {
            retain_lexer_pos = false;
            compile(&vm, program);
            print_vm_instructions(&vm);
            vm_dispatch(&vm);
            free_ast(program);
            printf("> ");
        } else {
            retain_lexer_pos = true;
            printf("... "); // Incomplete input; wait for more
        }

    }
    free_vm(&vm);
    free_string_table();
}

static void cleanup(ApexVM *vm, AST *ast, Parser *parser, char *source) {
    free_vm(vm);    
    free_ast(ast);
    free_parser(parser);
    apexLib_free();
    free_string_table();
    free(source);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        start_repl();
    } else if (argc == 2) {
        char *source = read_file(argv[1]);
        init_string_table();
        apexLib_init();

        Lexer lexer;
        init_lexer(&lexer, argv[1], source);

        Parser parser;
        init_parser(&parser, &lexer);
        parser.allow_incomplete = false;

        ApexVM vm;
        init_vm(&vm);
        
        AST *ast = parse_program(&parser);    
        if (!ast) {
            cleanup(&vm, ast, &parser, source);
            return EXIT_FAILURE;
        }
        print_ast(ast, 0);
        if (!compile(&vm, ast)) {
            cleanup(&vm, ast, &parser, source);
            return EXIT_FAILURE;
        }

        print_vm_instructions(&vm);
        if (!vm_dispatch(&vm)) {
            cleanup(&vm, ast, &parser, source);
            return EXIT_FAILURE;
        }
        cleanup(&vm, ast, &parser, source);
    } else {
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}