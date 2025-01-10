#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "vm.h"

extern bool compile(ApexVM *vm, AST *program);

#endif
