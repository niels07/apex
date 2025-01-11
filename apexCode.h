#ifndef APEX_CODE_H
#define APEX_CODE_H

#include "apexAST.h"
#include "apexVM.h"

extern bool apexCode_compile(ApexVM *vm, AST *program);

#endif
