#include <string.h>
#include <stdlib.h>
#include "apexVM.h"
#include "apexVal.h"
#include "apexStr.h"
#include "apexUtil.h"
#include "apexLib.h"
#include "apexErr.h"

int io_write(ApexVM *vm, int argc) {
    ApexValue value = apexVM_pop(vm);
    printf("%s", apexVal_tostr(value)->value);
    return 0;
}

int io_print(ApexVM *vm, int argc) {
    io_write(vm, argc);
    printf("\n");
    return 0;
}

int io_read(ApexVM *vm, int argc) {
    ApexString *line = apexUtil_readline(stdin);
    apexVM_pushval(vm, apexVal_makestr(line));
    return 0;
}

int file_write(ApexVM *vm, int argc) {
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

int io_open(ApexVM *vm, int argc) {
    ApexValue mode = apexVM_pop(vm);
    ApexValue filename = apexVM_pop(vm);    
    FILE *file = fopen(apexVal_str(filename)->value, apexVal_str(mode)->value);
    if (!file) {
        apexErr_runtime(vm, "cannot open file \"%s\"", apexVal_str(filename)->value);
        return 1;
    }
    ApexObject *obj = apexVal_newobject(apexStr_new("File", 4)->value);
    apexVal_objectset(obj, apexStr_new("__file_ptr", 10)->value, apexVal_makeptr(file));
    if (strchr(apexVal_str(mode)->value, 'r')) {
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
    ApexCfn write_fn = apexVal_newcfn(write_str->value, 1, file_write);
    apexVal_objectset(obj, write_str->value, apexVal_makecfn(write_fn));

    ApexString *close_str = apexStr_new("close", 5);
    ApexCfn close_fn = apexVal_newcfn(close_str->value, 0, file_close);
    apexVal_objectset(obj, close_str->value, apexVal_makecfn(close_fn));

    apexVM_pushval(vm, apexVal_makeobj(obj));
    return 0;
}

apex_reglib(io,
    apex_regfn("write", io_write, 1),
    apex_regfn("print", io_print, 1),
    apex_regfn("read", io_read, 0),
    apex_regfn("open", io_open, 2)
);