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




static void print_usage(void) {
    printf("Usage: apex [file]\n");
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    size_t size;
    char *buffer;

    if (!file) {
        apexErr_fatal("failed to open file: %s", path);
    }
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);
    buffer = mem_alloc(size + 1);
    fread(buffer, 1, size, file);
    fclose(file);
    buffer[size] = '\0';
    return buffer;
}

int main(int argc, char *argv[]) {
    const char *source;
    Lexer lexer;
    Parser parser;
    ApexVM vm;
    AST *ast;

    if (argc < 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    source = read_file(argv[1]);
    init_string_table();
    init_lexer(&lexer, source);
    init_parser(&parser, &lexer);
    
    ast = parse_program(&parser);    
    print_ast(ast, 0);
    init_vm(&vm);
    compile(&vm, ast);
    print_vm_instructions(&vm);
    vm_dispatch(&vm);

    free_vm(&vm);
    free_string_table();
    
    return EXIT_SUCCESS;
}