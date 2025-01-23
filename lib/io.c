#include <string.h>
#include <stdlib.h>
#include "apexVM.h"
#include "apexVal.h"
#include "apexStr.h"
#include "apexUtil.h"
#include "apexLib.h"
#include "apexErr.h"

/**
 * io:write writes a value to the standard output.
 *
 * Takes exactly 1 argument:
 * - The argument is a value to be converted to a string and printed.
 *
 * If the number of arguments is not exactly 1, an error is raised.
 */
int io_write(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "io:write expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    printf("%s", apexVal_tostr(value)->value);
    return 0;
}

/**
 * io:print writes a string to the standard output followed by a newline.
 *
 * Takes exactly 1 argument:
 * - The argument is a value to be converted to a string and printed.
 *
 * If the number of arguments is not exactly 1, an error is raised.
 */
int io_print(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "io:print expects exactly 1 argument");
        return 1;
    }
    io_write(vm, argc);
    printf("\n");
    return 0;
}

/**
 * io:read reads a line from the standard input and returns a string.
 *
 * Takes no arguments.
 *
 * If there is an error reading from the standard input, an error is raised.
 */
int io_read(ApexVM *vm, int argc) {
    ApexString *line = apexUtil_readline(stdin);
    apexVM_pushval(vm, apexVal_makestr(line));
    return 0;
}

/**
 * file.write writes a string to the file.
 *
 * The first argument is a file object returned by file:open.
 * The second argument is a string to write to the file.
 *
 * If the file is not open, an error is raised.
 */
int file_write(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "file.write expects exactly 1 argument");
        return 1;
    }
    ApexValue objval = apexVM_pop(vm);
    ApexValue text = apexVM_pop(vm);    
    ApexObject *file_obj = objval.objval;

    // Retrieve the FILE pointer from the object
    ApexValue file_ptr_val;
    if (!apexVal_objectget(&file_ptr_val, file_obj, apexStr_new("__file_ptr", 10)->value) || file_ptr_val.type != APEX_VAL_PTR) {
        apexErr_runtime(vm, "invalid file object");
        return 1;
    }

    FILE *file = (FILE *)file_ptr_val.ptrval;
    if (!file) {
        apexErr_runtime(vm, "file is not open");
        return 1;
    }

    fprintf(file, "%s", apexVal_str(text)->value);
    return 0;
}

/**
 * file:close closes a file.
 *
 * The argument is a file object returned by file:open.
 *
 * If the file is not open, an error is raised.
 *
 * After closing the file, the file object is modified to indicate that the file is not open.
 */
int file_close(ApexVM *vm, int argc) {
    ApexValue objval = apexVM_pop(vm);
    ApexObject *file_obj = objval.objval;

    // Retrieve the FILE pointer from the object
    ApexValue file_ptr_val;
    if (!apexVal_objectget(&file_ptr_val, file_obj, apexStr_new("__file_ptr", 10)->value) || file_ptr_val.type != APEX_VAL_PTR) {
        apexErr_runtime(vm, "invalid file object");
        return 1;
    }

    FILE *file = (FILE *)file_ptr_val.ptrval;
    if (!file) {
        apexErr_runtime(vm, "file is not open");
        return 1;
    }

    fclose(file);

    // Nullify the file pointer to indicate the file is closed
    apexVal_objectset(file_obj, apexStr_new("__file_ptr", 10)->value, apexVal_makeptr(NULL));
    return 0;
}

/**
 * io:open opens a file with the given name and mode.
 *
 * The first argument is the name of the file to open.
 * The second argument is the mode in which to open the file.
 * The mode is a string containing any of the following characters:
 *
 * r: open for reading
 * w: open for writing (truncates the file)
 * a: open for appending
 * x: open for reading and writing (fails if the file does not exist)
 * b: open in binary mode
 *
 * If the file is opened in read mode, the returned object will have a
 * "lines" property which is an array of strings, each of which is a line
 * from the file.
 *
 * The returned object will have a "write" property which is a function
 * that takes a single argument, a string to write to the file.
 *
 * The returned object will have a "close" property which is a function
 * that takes no arguments and closes the file.
 *
 * If the file could not be opened, the function will return null.
 */
int io_open(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "io:open expects exactly 2 arguments");
        return 1;
    }
    ApexValue mode_val = apexVM_pop(vm);

    if (apexVal_type(mode_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "second argument to io:open must be a string");
        return 1;
    }

    ApexValue filename_val = apexVM_pop(vm);    

    if (apexVal_type(filename_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "first argument to io:open must be a string");
        return 1;
    }

    char *mode = apexVal_str(mode_val)->value;
    char *filename = apexVal_str(filename_val)->value;

    FILE *file = fopen(filename, mode);
    if (!file) {
        apexVM_pushnull(vm);
        return 0;
    }
    ApexObject *obj = apexVal_newobject(apexStr_new("File", 4)->value);
    apexVal_objectset(obj, apexStr_new("__file_ptr", 10)->value, apexVal_makeptr(file));
    if (strchr(mode, 'r')) {
        ApexArray *lines = apexVal_newarray();
        char line[1024];
        int i = 0;
        while (fgets(line, sizeof(line), file)) {
            size_t len = strlen(line);
            if (line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            ApexValue index = apexVal_makeint(i++);
            apexVal_arrayset(lines, index, apexVal_makestr(apexStr_new(line, len)));
        }
        apexVal_objectset(obj, apexStr_new("lines", 5)->value, apexVal_makearr(lines));
        rewind(file);
    }
    ApexString *write_str = apexStr_new("write", 5);
    ApexCfn write_fn = apexVal_newcfn(write_str->value, file_write);
    apexVal_objectset(obj, write_str->value, apexVal_makecfn(write_fn));

    ApexString *close_str = apexStr_new("close", 5);
    ApexCfn close_fn = apexVal_newcfn(close_str->value, file_close);
    apexVal_objectset(obj, close_str->value, apexVal_makecfn(close_fn));

    apexVM_pushval(vm, apexVal_makeobj(obj));
    return 0;
}

apex_reglib(io,
    apex_regfn("write", io_write),
    apex_regfn("print", io_print),
    apex_regfn("read", io_read),
    apex_regfn("open", io_open)
);