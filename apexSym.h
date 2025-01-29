#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include "apexVal.h"

#define SYMBOL_TABLE_SIZE 1024

typedef unsigned int SymbolAddr;

/**
 * Represents a symbol in the symbol table.
 */
typedef struct Symbol {
    const char *name;      /** Name of the symbol */
    SymbolAddr addr;       /** Address of the symbol */
    ApexValue value;       /** Value associated with the symbol */
    struct Symbol *next;   /** Pointer to the next symbol in the linked list */
} Symbol;

/**
 * Represents a symbol table.
 */
typedef struct {
    Symbol **symbols;      /** Array of pointers to symbols */
    int count;             /** Number of symbols in the table */
    int size;              /** Size of the symbols array */
    float resize_threshold; /** Threshold for resizing the table */
} SymbolTable;

/**
 * Represents a local scope containing a symbol table.
 */
typedef struct LocalScope {
    SymbolTable table;     /** Symbol table for the local scope */
    struct LocalScope *next; /** Pointer to the next local scope */
    SymbolAddr next_addr;  /** Next available address for symbols */
} LocalScope;

/**
 * Represents a stack of local scopes.
 */
typedef struct ScopeStack {
    LocalScope *top;       /** Pointer to the top local scope on the stack */
} ScopeStack;

extern void init_symbol_table(SymbolTable *table);
extern void free_symbol_table(SymbolTable *table);
extern void apexSym_setglobal(SymbolTable *table, const char *name, ApexValue value);
extern bool apexSym_getglobal(ApexValue *value, SymbolTable *table, const char *name);
extern void init_scope_stack(ScopeStack *stack);
extern void push_scope(ScopeStack *stack);
extern void pop_scope(ScopeStack *stack);
extern void apexSym_setlocal(ScopeStack *stack, const char *name, ApexValue value);
extern bool apexSym_getlocal(ApexValue *value, ScopeStack *stack, const char *name);
extern void free_scope_stack(ScopeStack *stack);

#endif