#include <string.h>
#include <crypt.h>
#include <time.h>
#include "apexVM.h"
#include "apexMem.h"
#include "apexVal.h"
#include "apexLib.h"
#include "apexErr.h"

int crypt_hash(ApexVM *vm, int argc) {
    if (argc != 1) {
        apexErr_runtime(vm, "function 'crypt:hash' expects exactly 2 arguments");
        return 1;
    }

    ApexValue password_val = apexVM_pop(vm);
    if (apexVal_type(password_val) != APEX_VAL_STR) {
        apexErr_runtime(vm, "argument to 'crypt:hash' must be a string");
        return 1;
    }

    static const char charset[] = "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char salt[30];
    snprintf(salt, 30, "$2y$%02d$", 10);
    srand(time(NULL));
    for (int i = 0; i < 22; i++) {
        salt[7 + i] = charset[rand() % (sizeof(charset) - 1)];
    }
    salt[29] = '\0';

    const char *password_str = apexVal_str(password_val)->value;
    char *hash = crypt(password_str, salt);

    if (!hash) {
        apexErr_runtime(vm, "failed to hash the password");
        return 1;
    }

    ApexString *hash_str = apexStr_new(hash, strlen(hash));
    apexVM_pushstr(vm, hash_str);

    return 0;
}

apex_reglib(crypt,
    apex_regfn("hash", crypt_hash)
);