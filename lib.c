
#ifdef WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "lib.h"
#include "util.h"
#include "mem.h"
#include "string.h"

#define LIB_TABLE_SIZE 1024

typedef struct LibEntry {
    const char *key;
    ApexLibFn fn;
    struct LibEntry *next;
} LibEntry;

LibEntry *lib_table[LIB_TABLE_SIZE];

#define LIB_PATHS_MAX 16

static const char *LIB_PATHS[] = {
    "./",
    "./lib/",
    "/usr/lib/apex/",
    "/usr/local/lib/apex/",
    NULL
};

/**
 * Adds a function to the library table.
 *
 * This function creates a new entry in the library table with the specified
 * key and function pointer. The entry is inserted at the beginning of the
 * linked list at the hashed index in the table.
 *
 * @param key The key associated with the function to be added.
 * @param fn The function pointer to be stored in the library table.
 */
void apexLib_add(const char *libname, const char *fnname, ApexLibFn fn) {
    char key[1024];
    snprintf(key, sizeof(key), "%s:%s", libname, fnname);
    unsigned int index = apexUtl_hash(key) % LIB_TABLE_SIZE;
    LibEntry *entry = apexMem_alloc(sizeof(LibEntry));
    printf("adding %s at %d\n", key, index);
    entry->key = apexStr_new(key, strlen(key))->value;
    entry->fn = fn;
    entry->next = lib_table[index];
    lib_table[index] = entry;
}

/**
 * Retrieves a function from the library table by its library name and
 * function name.
 *
 * This function takes a library name and a function name as input and
 * attempts to find an entry in the library table with the same library
 * name and function name. If such an entry is found, the function returns
 * the function pointer associated with the entry, otherwise it returns
 * NULL.
 *
 * @param libname The name of the library to search for.
 * @param fnname The name of the function to search for.
 * @return The function pointer associated with the library name and function
 *         name if found, otherwise NULL.
 */
ApexLibFn apexLib_get(const char *libname, const char *fnname) {
    char key[1024];
    snprintf(key, sizeof(key), "%s:%s", libname, fnname);
    ApexString *keystr = apexStr_new(key, strlen(key));
    unsigned int index = apexUtl_hash(key) % LIB_TABLE_SIZE;
    LibEntry *entry = lib_table[index];
    while (entry) {
        if (entry->key == keystr->value) {
            return entry->fn;
        }
        entry = entry->next;
    }
    return NULL;
}

static void load_shared_library(const char *libpath, const char *libname) {
    char openfn_name[256];
    snprintf(openfn_name, sizeof(openfn_name), "apex_register_%s", libname);

#ifdef WIN32
    HMODULE handle = LoadLibrary(libpath);
    if (!handle) {
        fprintf(stderr, "Error loading library %s: %lu\n", libpath, GetLastError());
        return;
    }

    // Find the registration function
    FARPROC register_fn = GetProcAddress(handle, openfn_name);
    if (!register_fn) {
        fprintf(stderr, "Error finding symbol %s: %lu\n", openfn_name, GetLastError());
        FreeLibrary(handle);
        return;
    }

    // Call the registration function
    ((void (*)(void))register_fn)();
#else
    // Open the shared library
    void *handle = dlopen(libpath, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading library %s: %s\n", libpath, dlerror());
        return;
    }

    // Find the registration function
    void (*register_fn)(void) = dlsym(handle, openfn_name);
    if (!register_fn) {
        fprintf(stderr, "Error finding symbol %s: %s\n", openfn_name, dlerror());
        dlclose(handle);
        return;
    }

    // Call the registration function
    register_fn();
#endif
}

static void get_search_paths(const char **paths) {
    char *env_path = getenv("APEX_PATH");
    if (env_path) {
        char *token = strtok(env_path, ":");
        int i = 0;
        while (token && i < LIB_PATHS_MAX - 1) {
            paths[i++] = apexStr_new(token, strlen(token))->value;
            token = strtok(NULL, ":");
        }
        paths[i] = NULL;
    } else {
        memcpy(paths, LIB_PATHS, sizeof(LIB_PATHS));
    }
}

static void load_path_libs(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".so")) { // Look for .so files
            char lib_path[1024];
            snprintf(lib_path, sizeof(lib_path), "%s/%s", path, entry->d_name);

            // Extract module name: lib<name>.so -> <name>
            char *libname = entry->d_name + 3; // Skip "lib"
            char *ext = strstr(libname, ".so");
            if (ext) {
                *ext = '\0';
            }

            // Load the library
            load_shared_library(lib_path, libname);
        }
    }
    closedir(dir);
}


void apexLib_init(void) {
    const char *paths[LIB_PATHS_MAX];
    get_search_paths(paths);

    for (int i = 0; paths[i] != NULL; i++) {
        load_path_libs(paths[i]);
    }
}

