#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "compiler.h"
#include "io.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "vm.h"
#include "hash.h"

#define HASH_TABLE_INIT_SIZE 1024

typedef enum {
    MODE_BUILD = 1 << 0,
    MODE_RUN   = 1 << 1,
    MODE_PRINT = 1 << 2,
    MODE_DEBUG = 1 << 3
} Mode;

typedef struct options {
    Mode mode;
    char *input_filename;
    char *output_filename;
    char **argv;
    char *arg;
    int argc;
} Options;

#define NEXT_ARG(opts) (opts->arg = *++opts->argv)

static void build_file(Options *opts) {
    ApexStream *stream = apex_stream_open(opts->input_filename);
    ApexLexer *lexer = apex_lexer_new(stream);
    ApexAst *ast = apex_ast_new();
    ApexParser *parser = apex_parser_new(lexer, ast);
    ApexHashTable *table = apex_hash_table_new(HASH_TABLE_INIT_SIZE);
    ApexCompiler *compiler = apex_compiler_new(ast, table);

    apex_parser_parse(parser); 
    apex_compiler_compile(compiler, opts->output_filename); 
}

static int opt_set(Options *options, Mode mode) {
    return options->mode & mode;
}

static void exec_file(Options *opts) {
    ApexHashTable *table = apex_hash_table_new(HASH_TABLE_INIT_SIZE);
    ApexVm *vm = apex_vm_new(table, opts->input_filename);

    extern void apex_api_init(ApexVm *);
    apex_api_init(vm);
    if (opt_set(opts, MODE_DEBUG)) {
        apex_vm_set_debug(vm, 1);
    }
    apex_vm_dispatch(vm);
}

static void print_code(Options *opts) {
    ApexHashTable *table = apex_hash_table_new(HASH_TABLE_INIT_SIZE);
    ApexVm *vm = apex_vm_new(table, opts->input_filename);
    apex_vm_print(vm);
}

static void set_opt(Options *options, Mode mode) {
    options->mode |= mode;
}

static void init_opts(Options *opts, int argc, char **argv) {
    opts->input_filename = NULL;
    opts->output_filename = NULL; 
    opts->mode = 0;
    opts->argc = argc;
    opts->argv = argv;
}

static int check_opt(Options *opts, const char *arg, Mode mode) {
    if (!opts->arg) {
        return 0;
    }
    if (strcmp(opts->arg, arg) != 0) {
        return 0;
    }
    set_opt(opts, mode);
    NEXT_ARG(opts);
    return 1;
}

static int accept_arg(Options *opts, const char *arg) {
    if (arg && strcmp(opts->arg, arg) == 0) {
        NEXT_ARG(opts);
        return 1;
    }
    return 0;
}

static char *get_opt_arg(Options *opts, const char *opt) {
    if (!accept_arg(opts, opt)) {
        return NULL;
    }
    if (!opts->arg) {
        apex_error_argument("missing argument for '%s'", opt);
    }
    return opts->arg;
}

static void get_input_filename(Options *opts) {
    if (opts->input_filename || *opts->arg == '-') {
        apex_error_argument("invalid argument '%s'", opts->arg);     
    }
    opts->input_filename = opts->arg;
}

static void handle_args(Options *opts) {
    for (NEXT_ARG(opts); opts->arg; NEXT_ARG(opts)) {
        check_opt(opts, "build", MODE_BUILD);
        check_opt(opts, "run", MODE_RUN);
        check_opt(opts, "print", MODE_PRINT);
        check_opt(opts, "debug", MODE_DEBUG);

        if (!opts->arg) {
            break;
        }
        opts->output_filename = get_opt_arg(opts, "-o");
        if (!opts->output_filename) {
            get_input_filename(opts);     
        }
    }
    if (!opts->output_filename) {
        opts->output_filename = "apex.out"; 
    }
}

void argument_error(ApexErrorHandler *handler) {
    apex_error_print(handler);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    Options opts;

    if (argc < 2) {
        return 0;
    }
    apex_error_set_handler(APEX_ERROR_CODE_ARGUMENT, argument_error, NULL);

    init_opts(&opts, argc, argv);
    handle_args(&opts);

    if (opt_set(&opts, MODE_PRINT)) {
        print_code(&opts);
        return 0;
    }

    if (opt_set(&opts, MODE_BUILD)) {
        build_file(&opts);
        opts.input_filename = opts.output_filename;
    }

    if (opt_set(&opts, MODE_RUN)) {
        exec_file(&opts);
    }
    return 0;
}

