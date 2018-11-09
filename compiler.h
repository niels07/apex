#ifndef APEX_COMPILER_H
#define APEX_COMPILER_H

#include "ast.h"
#include "hash.h"

typedef struct ApexCompiler ApexCompiler;

ApexCompiler *apex_compiler_new(ApexAst *, ApexHashTable *);

extern void apex_compiler_compile(ApexCompiler *, const char *);

#endif
