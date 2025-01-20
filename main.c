#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include "apexLex.h"
#include "apexStr.h"
#include "apexMem.h"
#include "apexErr.h"
#include "apexParse.h"
#include "apexVM.h"
#include "apexVal.h"
#include "apexCode.h"
#include "apexLib.h"

#define INPUT_BUFFER_SIZE 1024
#define HISTORY_INIT_SIZE 32

static int history_count = 0;
static int history_index = -1;
static int history_size = HISTORY_INIT_SIZE;
static char **history;
static struct termios old_termios;
static struct termios new_termios;

static void init_terminal() {
    tcgetattr(0, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(0, TCSANOW, &new_termios);    
}

static void reset_terminal() {
    tcsetattr(0, TCSANOW, &old_termios);
}

static void init_history(void) {
    history = apexMem_alloc(history_size * sizeof(char *));
}

void add_to_history(const char *line) {
    if (history_count == history_size) {
        history_size *= 2;
        history = apexMem_realloc(history, history_size * sizeof(char *));
    }
    size_t len = strlen(line);
    history[history_count] = apexMem_alloc(len + 1);
    strncpy(history[history_count], line, len);
    history[history_count][len] = '\0';
    history_count++;
}

static void free_history(void) {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    free(history);
}

static void clear_line() {
    printf("\r\033[K");
}

void redraw_prompt(const char *input, const char *prompt) {
    clear_line();
    printf("%s %s", prompt, input);
    fflush(stdout);
}

static void read_line(char *input, size_t size, bool is_incomplete) {
    size_t pos = 0;
    int c;
    memset(input, 0, size);

    for (;;) {
        c = getchar();

        if (c == '\n') {
            putchar('\n');
            input[pos] = '\0';
            if (pos > 0) {
                add_to_history(input);
            }
            input[pos] = '\n';
            history_index = history_count; // Reset index after enter
            return;
        } else if (c == 127) { // Backspace
            if (pos > 0) {
                pos--;
                printf("\b \b");
            }
        } else if (c == '\033') { // Arrow keys
            getchar(); // Skip '['
            switch (getchar()) {
            case 'A': // Up arrow
                if (history_index > 0) {
                    history_index--;
                    strcpy(input, history[history_index]);                    
                    pos = strlen(input);
                    redraw_prompt(input, is_incomplete ? "..." : ">");
                }
                break;
            case 'B': // Down arrow
                if (history_index < history_count - 1) {
                    history_index++;
                    strcpy(input, history[history_index]);
                } else {
                    history_index = history_count;
                    input[0] = '\0';
                }
                pos = strlen(input);
                redraw_prompt(input, is_incomplete ? "..." : ">");
                break;
            }
        } else if (pos < size - 1) { // Regular characters
            input[pos++] = c;
            input[pos] = '\0';
            putchar(c);
        }
    }
}

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
    bool is_incomplete = false;

    apexStr_inittable();
    apexLib_init();
    init_lexer(&lexer, NULL, NULL);

    ApexVM vm;
    init_vm(&vm);
    int lexer_pos = 0;
    bool retain_lexer_pos = false;

    init_terminal();
    init_history();

    printf("> ");
    for(;;) {
        read_line(input, sizeof(input), is_incomplete);
        apexLex_feedline(&lexer, input);
        
        if (!retain_lexer_pos) {
            lexer_pos = lexer.position;
        } else {
            lexer.position = lexer_pos;
        }
        init_parser(&parser, &lexer);
        parser.allow_incomplete = true;

        AST *program = parse_program(&parser);
        #ifdef DEBUG
        print_ast(program, 0);
        #endif
        if (program) {
            is_incomplete = false;
            retain_lexer_pos = false;
            if (program->type != AST_ERROR) {
                apexCode_compile(&vm, program);
                #ifdef DEBUG
                print_vm_instructions(&vm);
                #endif
                vm_dispatch(&vm);
            }
            free_ast(program);
            apexVM_reset(&vm);
            printf("> ");
        } else {
            is_incomplete = true;
            retain_lexer_pos = true;
            printf("... "); // Incomplete input; wait for more
        }

    }
    reset_terminal();
    free_vm(&vm);
    apexStr_freetable();
    free_history();
}

static void cleanup(ApexVM *vm, AST *ast, Parser *parser, char *source) {
    free_vm(vm);    
    free_ast(ast);
    free_parser(parser);
    apexLib_free();
    apexStr_freetable();
    free(source);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        start_repl();
    } else if (argc >= 2) {
        char *source = read_file(argv[1]);
        apexStr_inittable();
        apexLib_init();

        Lexer lexer;
        init_lexer(&lexer, argv[1], source);

        Parser parser;
        init_parser(&parser, &lexer);
        parser.allow_incomplete = false;

        ApexVM vm;
        init_vm(&vm);

        ApexArray *args = apexVal_newarray();
        int argi = 0;
        for (int i = 1; i < argc; i++) {
            ApexValue arg_index = apexVal_makeint(argi++);
            ApexValue arg_value = apexVal_makestr(apexStr_new(argv[i], strlen(argv[i])));
            apexVal_arrayset(args, arg_index, arg_value);
        }

        apexSym_setglobal(
            &vm.global_table, 
            apexStr_new("@args", 5)->value, 
            apexVal_makearr(args));
        
        AST *ast = parse_program(&parser);    
        if (!ast) {
            cleanup(&vm, ast, &parser, source);
            return EXIT_FAILURE;
        }
        #ifdef DEBUG
        print_ast(ast, 0);
        #endif
        if (!apexCode_compile(&vm, ast)) {
            cleanup(&vm, ast, &parser, source);
            return EXIT_FAILURE;
        }
        #ifdef DEBUG
        print_vm_instructions(&vm);
        #endif
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