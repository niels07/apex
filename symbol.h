#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "value.h"
#include "bool.h"

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
} LocalScope;

typedef struct ScopeStack {
    LocalScope *top;
} ScopeStack;

extern void init_symbol_table(SymbolTable *table);
extern void free_symbol_table(SymbolTable *table);
extern SymbolAddr add_symbol(SymbolTable *table, const char *name);
extern SymbolAddr add_symbol_with_value(SymbolTable *table, const char *name, ApexValue value);
extern Bool set_symbol_value(SymbolTable *table, const char *name, ApexValue value);
extern SymbolAddr get_symbol_address(SymbolTable *table, const char *name);
extern ApexValue get_symbol_value_by_addr(SymbolTable *table, SymbolAddr addr);
extern ApexValue get_symbol_value_by_name(SymbolTable *table, const char *name);
extern void init_scope_stack(ScopeStack *stack);
extern void push_scope(ScopeStack *stack);
extern void pop_scope(ScopeStack *stack);
extern SymbolAddr add_local_symbol(ScopeStack *stack, const char *name);
extern Bool set_local_symbol_value(ScopeStack *stack, SymbolAddr addr, ApexValue value);
extern ApexValue get_local_symbol_value(ScopeStack *stack, SymbolAddr addr);
extern SymbolAddr get_local_symbol_address(ScopeStack *stack, const char *name);
extern void free_scope_stack(ScopeStack *stack);

#endif