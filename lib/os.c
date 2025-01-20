
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "apexVM.h"
#include "apexErr.h"
#include "apexLib.h"
#include "apexMem.h"

int os_exit(ApexVM *vm, int argc) {
    if (argc == 0) {
        exit(EXIT_SUCCESS);
    }
    if (argc != 1) {
        apexErr_runtime(vm, "os:exit expects at most 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    if (apexVal_type(value) != APEX_VAL_INT) {
        apexErr_runtime(vm, "os:exit expects argument to be int");
        return 1;
    }
    exit(apexVal_int(value));
    return 0;
}

int os_remove(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "os:remove expects exactly 1 argument");
        return 1;
    }
    ApexValue value = apexVM_pop(vm);
    if (apexVal_type(value) != APEX_VAL_STR) {
        apexErr_runtime(vm, "os:remove expects argument to be string");
        return 1;
    }
    char *filename = apexVal_str(value)->value;
    if (remove(filename) != 0) {
        apexErr_runtime(vm, "could not remove %s", filename);
        return 1;
    }
    return 0;
}

int os_rename(ApexVM *vm, int argc) {
    if (argc != 2) {
        apexErr_runtime(vm, "os:rename expects exactly 2 arguments");
        return 1;
    }
    ApexValue new_val = apexVM_pop(vm);
    if (apexVal_type(new_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "second argument to os:rename must be a string");
        return 1;
    }
    ApexValue old_val = apexVM_pop(vm);
    if (apexVal_type(old_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "first argument to os:rename must be a string");
        return 1;
    }
    char *new = apexVal_str(new_val)->value;
    char *old = apexVal_str(old_val)->value;
    if (rename(old, new) != 0) {
        apexErr_runtime(vm, "could not rename file %s to %s", old, new);
        return 1;
    }
    return 0;
}

int os_time(ApexVM *vm, int argc) {
    if (argc > 1) {
        apexErr_runtime(vm, "os:time expects at most 1 argument");
        return 1;
    }
    struct tm timeinfo = {0};
    timeinfo.tm_year = 1970;
    timeinfo.tm_mon = 1;
    timeinfo.tm_mday = 1;
    timeinfo.tm_hour = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_sec = 0;  

    if (argc == 1) {
        ApexValue time_array_val = apexVM_pop(vm);
        if (apexVal_type(time_array_val) != APEX_VAL_ARR) {
            apexErr_runtime(vm, "os:time expects argument to be an array");
            return 1;
        }
        ApexValue value;
        ApexArray *time_array = apexVal_array(time_array_val);
        #define SET_FIELD_IF_EXISTS(key, field) do { \
            ApexValue value; \
            if (apexVal_arrayget(&value, time_array, apexVal_makestr(apexStr_new(key, strlen(key))))) { \
                if (value.type != APEX_VAL_INT) { \
                    apexErr_runtime(vm, "array field '%s' is not an integer", key); \
                    return 1; \
                } \
                field = apexVal_int(value); \
            } \
        } while (0)
        SET_FIELD_IF_EXISTS("year", timeinfo.tm_year);
        SET_FIELD_IF_EXISTS("month", timeinfo.tm_mon);
        SET_FIELD_IF_EXISTS("day", timeinfo.tm_mday);
        SET_FIELD_IF_EXISTS("hour", timeinfo.tm_hour);
        SET_FIELD_IF_EXISTS("min", timeinfo.tm_min);
        SET_FIELD_IF_EXISTS("sec", timeinfo.tm_sec);        
    }
    timeinfo.tm_year -= 1900;
    timeinfo.tm_mon -= 1;
    time_t result = mktime(&timeinfo);
    if (result == -1) {
        apexErr_runtime(vm, "os:time failed to compute time");
        return 1;
    }
    apexVM_pushint(vm, (int)result);
    return 0;
}

int os_date(ApexVM *vm, int argc) {
    if (argc < 1 || argc > 2) {
        apexErr_runtime(vm, "os:date expects 1 or 2 arguments");
        return 1;
    }

    ApexValue time_arg;
    time_t raw_time;
    // Check if an optional time argument is provided
    if (argc > 1) {
        time_arg = apexVM_pop(vm);
        if (apexVal_type(time_arg) != APEX_VAL_INT) {
            apexErr_runtime(vm, "os:date expects an int as second argument");
            return 1;
        }
        raw_time = (time_t)apexVal_int(time_arg);
    } else {
        raw_time = time(NULL);
    }

    ApexValue format_arg = apexVM_pop(vm);

    if (apexVal_type(format_arg) != APEX_VAL_STR) {
        apexErr_runtime(vm, "os:date expects a string as the first argument");
        return 1;
    }
    const char *format = apexVal_str(format_arg)->value;

    // Convert the time_t to a struct tm
    struct tm *timeinfo = localtime(&raw_time);
    if (!timeinfo) {
        apexErr_runtime(vm, "os:date failed to retrieve time information");
        return 1;
    }

    size_t buffer_size = 256;
    char *buffer = apexMem_alloc(buffer_size);

    for (;;) {
        size_t written = strftime(buffer, buffer_size, format, timeinfo);
        if (written > 0) {
            break;
        }
        buffer_size *= 2;
        buffer = apexMem_realloc(buffer, buffer_size);
    }

    ApexString *formatted_str = apexStr_save(buffer, strlen(buffer));
    apexVM_pushstr(vm, formatted_str);
    return 0;
}

apex_reglib(os,
    apex_regfn("exit", os_exit),
    apex_regfn("remove", os_remove),
    apex_regfn("rename", os_rename),
    apex_regfn("time", os_time),
    apex_regfn("date", os_date)
)