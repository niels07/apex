#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "hash.h"
#include "api.h"

#define HASH_TABLE_INIT_SIZE 1024

typedef enum {
    MODE_BUILD,
    MODE_RUN,
    MODE_PRINT
} build_mode;

typedef struct options {
    build_mode mode;
    char *input_filename;
    char *output_filename;
    char **argv;
    char *arg;
    int argc;
} Options;

#define NEXT_ARG(opts) (opts->arg = *++opts->argv)

static FILE *open_file(const char *filename, const char *type) {
    FILE *file = fopen(filename, type);
    if (file == NULL) {
        ApexError_io("could not open %s", filename);
    }
    return file;
}

static void build_file(Options *opts) {
    FILE *output_file;
    FILE *input_file;
    ApexHash_table *table;
    ApexLexer *lexer;

    input_file = open_file(opts->input_filename, "r");
    lexer = ApexLexer_new(input_file);
    if (!input_file) return;
    table = ApexHash_create_table(HASH_TABLE_INIT_SIZE);

    output_file = open_file(opts->output_filename, "w");
    if (!output_file) {
        fclose(input_file);
        return;
    }

    ApexCompiler_write(lexer, output_file);

    ApexLexer_free(lexer);
    ApexHash_free_table(table);
    fclose(input_file);
    fclose(output_file);
}

static int opt_set(Options *options, build_mode mode) {
    return options->mode & mode;
}

static void exec_file(Options *opts) {
    ApexHash_table *table;
    ApexVm *vm;
    
    table = ApexHash_create_table(HASH_TABLE_INIT_SIZE);
    vm = ApexVm_new(table, opts->input_filename);
    ApexVm_dispatch(vm);
}

static void set_opt(Options *options, build_mode mode) {
    options->mode |= mode;
}

static void init_opts(Options *opts, int argc, char **argv) {
    opts->input_filename = NULL;
    opts->output_filename = NULL; 
    opts->mode = 0;
    opts->argc = argc;
    opts->argv = argv;
}

static int check_opt(Options *opts, const char *arg, build_mode mode) {
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
        ApexError_argument("missing argument for '%s'", opt);
    }
    return opts->arg;
}

static void get_input_filename(Options *opts) {
    if (opts->input_filename || *opts->arg == '-') {
        ApexError_argument("invalid argument '%s'", opts->arg);     
    }
    opts->input_filename = opts->arg;
}

static void handle_args(Options *opts) {
    for (NEXT_ARG(opts); opts->arg; NEXT_ARG(opts)) {
        check_opt(opts, "build", MODE_BUILD);
        check_opt(opts, "run", MODE_RUN);
        check_opt(opts, "print", MODE_PRINT);

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
    ApexError_print(handler);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    Options opts;

    if (argc < 2) {
        return 0;
    }
    ApexError_set_handler(APEX_ERROR_CODE_ARGUMENT, argument_error, NULL);

    init_opts(&opts, argc, argv);
    handle_args(&opts);

    if (opt_set(&opts, MODE_BUILD)) {
        build_file(&opts);
        opts.input_filename = opts.output_filename;
    }

    if (opt_set(&opts, MODE_RUN)) {
        exec_file(&opts);
    }
    return 0;
}

