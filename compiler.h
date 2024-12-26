#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "vm.h"

void compile(ApexVM *vm, AST *program);

#endif
