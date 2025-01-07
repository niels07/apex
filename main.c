#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "string.h"
#include "mem.h"
#include "error.h"
#include "parser.h"
#include "vm.h"
#include "value.h"
#include "compiler.h"
#include "lib.h"

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    char *source = read_file(argv[1]);
    init_string_table();
    apexLib_init();

    Lexer lexer;
    init_lexer(&lexer, argv[1], source);

    Parser parser;
    init_parser(&parser, &lexer);
    
    AST *ast = parse_program(&parser);    
    print_ast(ast, 0);

    ApexVM vm;
    init_vm(&vm);
    compile(&vm, ast);

    print_vm_instructions(&vm);
    vm_dispatch(&vm);
    
    free_vm(&vm);    
    free_ast(ast);
    free_parser(&parser);
    free_string_table();
    free(source);
    
    return EXIT_SUCCESS;
}