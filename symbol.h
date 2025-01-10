#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include "apexVal.h"

#define SYMBOL_TABLE_SIZE 1024

typedef unsigned int SymbolAddr;

typedef struct Symbol {
    const char *name;
    SymbolAddr addr;
    ApexValue value;
    struct Symbol *next;
} Symbol;

typedef struct {
    Symbol **symbols;
    int count;
    int size;
    float resize_threshold;
} SymbolTable;

typedef struct LocalScope {
    SymbolTable table;
    struct LocalScope *next;
    SymbolAddr next_addr;
} LocalScope;

typedef struct ScopeStack {
    LocalScope *top;
} ScopeStack;

extern void init_symbol_table(SymbolTable *table);
extern void free_symbol_table(SymbolTable *table);
extern SymbolAddr add_symbol(SymbolTable *table, const char *name);
extern SymbolAddr add_symbol_with_value(SymbolTable *table, const char *name, ApexValue value);
extern void apexSym_setglobal(SymbolTable *table, const char *name, ApexValue value);
extern SymbolAddr get_symbol_address(SymbolTable *table, const char *name);
extern ApexValue get_symbol_value_by_addr(SymbolTable *table, SymbolAddr addr);
extern bool apexSym_getglobal(ApexValue *value, SymbolTable *table, const char *name);
extern void init_scope_stack(ScopeStack *stack);
extern void push_scope(ScopeStack *stack);
extern void pop_scope(ScopeStack *stack);
extern SymbolAddr add_local_symbol(ScopeStack *stack, const char *name);
extern void apexSym_setlocal(ScopeStack *stack, const char *name, ApexValue value);
extern bool apexSym_getlocal(ApexValue *value, ScopeStack *stack, const char *name);
extern SymbolAddr get_local_symbol_address(ScopeStack *stack, const char *name);
extern void free_scope_stack(ScopeStack *stack);

#endif