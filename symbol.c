#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "symbol.h"
#include "mem.h"
#include "string.h"
#include "value.h"

/**
 * Computes a hash value for a given string.
 *
 * This function is a simple string hashing algorithm, which iteratively
 * processes each character in the string with a specified step size and
 * combines them using a bitwise operation. The resulting hash value is
 * then reduced modulo the size of the symbol table to fit within the table.
 *
 * @param str The input string to be hashed.
 * @return An unsigned integer representing the hash value of the input string.
 */
static unsigned int hash_string(const char *str) {
    unsigned int hash = 0;
    while (*str) {
        hash = (hash << 5) + hash + (unsigned char)(*str);
        str++;
    }
    return hash;
}

/**
 * Resizes the symbol table to twice its current size.
 *
 * This function is called when the load factor of the symbol table exceeds a
 * certain threshold (75%). It allocates a new array of symbols with twice the
 * size of the current one, and rehashes all the symbols in the table to their new
 * positions in the new array. Finally, it frees the old array and sets the
 * symbol table's size to the new size.
 *
 * @param table A pointer to the symbol table to resize.
 */
static void resize_symbol_table(SymbolTable *table) {
    unsigned int new_size = table->size * 2;
    Symbol **symbols = apexMem_calloc(new_size, sizeof(Symbol *));
    int i;
    
    for (i = 0; i < table->size; i++) {
        Symbol *current = table->symbols[i];
        while (current) {
            Symbol *next = current->next;
            unsigned int hash = hash_string(current->name) % new_size;
            current->next = symbols[hash];
            symbols[hash] = current;
            current = next;
        }
    }
    
    free(table->symbols);
    table->symbols = symbols;
    table->size = new_size;
}


/**
 * Initializes a symbol table.
 *
 * This function sets up a symbol table with an initial size and count,
 * and allocates memory for its symbols. The resize threshold is set to
 * 75% of the table's capacity, indicating when the table should be resized
 * to maintain efficiency.
 *
 * @param table A pointer to the symbol table to initialize.
 */
void init_symbol_table(SymbolTable *table) {
    table->size = 8;
    table->count = 0;
    table->resize_threshold = 0.75f;
    table->symbols = apexMem_calloc(table->size, sizeof(Symbol *));
}


/**
 * Adds a new symbol to the symbol table.
 *
 * This function checks if the symbol table needs resizing based on its
 * load factor, and resizes it if necessary. A new symbol is then created
 * with a given name and a default null value, and is inserted into the
 * table at the hashed index. The symbol's address is returned.
 *
 * @param table The symbol table to add the symbol to.
 * @param name The name of the symbol to add.
 * @return The address of the newly added symbol.
 */
SymbolAddr add_symbol(SymbolTable *table, const char *name) {
    if ((float)table->count / table->size > table->resize_threshold) {
        resize_symbol_table(table);
    }
    
    unsigned int hash = hash_string(name) % table->size;
    Symbol *symbol = apexMem_alloc(sizeof(Symbol));
    symbol->name = name;
    symbol->addr = hash_string(name);
    symbol->value = apexVal_makenull();
    symbol->next = table->symbols[hash];

    table->symbols[hash] = symbol;
    table->count++;
    return symbol->addr;
}

/**
 * Adds a new symbol to the table, with an initial value.
 *
 * If the table has reached its resize threshold (75% full), the table is
 * resized to double its current size before the new symbol is added.
 *
 * @param table The symbol table to add the symbol to.
 * @param name The name of the symbol to add.
 * @param value The value to associate with the symbol.
 * @return The address of the newly added symbol.
 */
SymbolAddr add_symbol_with_value(SymbolTable *table, const char *name, ApexValue value) {
    unsigned int hash;
    Symbol *symbol;

    if ((float)table->count / table->size > table->resize_threshold) {
        resize_symbol_table(table);
    }
    
    hash = hash_string(name) % table->size;
    symbol = apexMem_alloc(sizeof(Symbol));
    symbol->name = name;
    symbol->addr = hash_string(name);
    symbol->value = value;
    symbol->next = table->symbols[hash];
    table->symbols[hash] = symbol;
    table->count++;
    return symbol->addr;
}


void apexSym_setglobal(SymbolTable *table, const char *name, ApexValue value) {
    unsigned int hash = hash_string(name) % table->size;
    Symbol *current = table->symbols[hash];

    while (current) {
        if (current->name == name) {
            ApexValue oldval = current->value;
            apexVal_release(oldval);
            apexVal_retain(value);
            current->value = value;
            return;
        }
        current = current->next;
    }
    Symbol *symbol = apexMem_alloc(sizeof(Symbol));
    symbol->name = name;
    symbol->addr = hash_string(name);
    symbol->value = value;
    symbol->next = table->symbols[hash];
    table->symbols[hash] = symbol;
    table->count++;
}

/**
 * Gets the address of a symbol in the symbol table.
 *
 * This function walks the linked list of symbols at the hashed index
 * and looks up the symbol by name. If the symbol is found, its address is
 * returned. If the symbol is not found, -1 is returned.
 *
 * @param table The symbol table to search.
 * @param name The name of the symbol to look up.
 * @return The address of the symbol, or -1 if the symbol is not found.
 */
SymbolAddr get_symbol_address(SymbolTable *table, const char *name) {
    unsigned int hash = hash_string(name) % table->size;
    Symbol *current = table->symbols[hash];
    
    while (current) {
        if (current->name == name) {
            return current->addr;
        }
        current = current->next;
    }
    
    return -1;
}

/**
 * Retrieves the value associated with a given symbol address from the symbol
 * table.
 *
 * @param table The symbol table to search.
 * @param addr The symbol address to look up.
 *
 * @return The value associated with the given symbol address, or NULL if no
 *         such symbol exists.
 */
ApexValue get_symbol_value_by_addr(SymbolTable *table, SymbolAddr addr) {
    int i;
    for (i = 0; i < table->size; i++) {
        Symbol *current = table->symbols[i];
        while (current) {
            if (current->addr == addr) {
                return current->value;
            }
            current = current->next;
        }
    }
    return apexVal_makenull();
}

/**
 * Retrieves the value associated with a given symbol name from the symbol table.
 *
 * This function walks the linked list of symbols at the hashed index
 * and looks up the symbol by name. If the symbol is found, its associated value
 * is returned. If the symbol is not found, a null value is returned.
 *
 * @param table The symbol table to search.
 * @param name The name of the symbol to look up.
 *
 * @return The value associated with the given symbol name, or a null value if
 *         no such symbol exists.
 */
bool apexSym_getglobal(ApexValue *value, SymbolTable *table, const char *name) {
    unsigned int hash = hash_string(name) % table->size;
    Symbol *current = table->symbols[hash];
    
    while (current) {
        if (current->name == name) {
            *value = current->value;
            return true;
        }
        current = current->next;
    }
    
    return false;
}

/**
 * Frees the memory allocated for the symbol table.
 *
 * This function iterates over each entry in the table, freeing the memory
 * allocated for the Symbol structure and the Value structure that it holds.
 * It then sets the entry in the table to NULL. Finally, it frees the memory
 * allocated for the table itself and resets its size and count to 0.
 *
 * @param table A pointer to the SymbolTable to be freed.
 */
void free_symbol_table(SymbolTable *table) {
    int i;
    for (i = 0; i < table->size; i++) {
        Symbol *current = table->symbols[i];
        while (current) {
            Symbol *next = current->next;
            apexVal_release(current->value);
            free(current);
            current = next;
        }
    }
    free(table->symbols);
    table->symbols = NULL;
    table->size = 0;
    table->count = 0;
}

/**
 * Pushes a new local scope onto the stack.
 *
 * This function allocates a new LocalScope on the heap, initializes its symbol
 * table, and sets its next pointer to the current top of the stack. It then
 * sets the new scope as the top of the stack.
 *
 * @param stack The stack to push onto.
 */
void push_scope(ScopeStack *stack) {
    LocalScope *scope = apexMem_alloc(sizeof(LocalScope));
    init_symbol_table(&scope->table);
    scope->next = stack->top;
    scope->next_addr = 1;
    stack->top = scope;
}

/**
 * Pops the top local scope off the stack.
 *
 * This function sets the top of the stack to the next scope down, and frees
 * the memory allocated for the current top scope and its symbol table.
 *
 * @param stack The stack to pop from.
 */
void pop_scope(ScopeStack *stack) {
    if (stack->top) {
        LocalScope *old_top = stack->top;
        stack->top = old_top->next;
        free_symbol_table(&old_top->table);
        free(old_top);
    }
}

/**
 * Adds a local symbol to the stack.
 *
 * If the stack is not empty, this function adds a symbol to the top scope's
 * symbol table. If the stack is empty, this function does nothing.
 *
 * @param stack The stack to add the local symbol to.
 * @param name The name of the local symbol.
 * @param addr The address of the local symbol.
 */
SymbolAddr add_local_symbol(ScopeStack *stack, const char *name) {
    if (stack->top) {
        return add_symbol(&stack->top->table, name);
    }
    return -1;
}


/**
 * Sets the value of a local symbol in the current scope.
 *
 * This function walks the linked list of symbols at the hashed index
 * and looks up the symbol by name. If the symbol is found, its value is
 * updated. If the symbol is not found, the symbol is added to the table.
 *
 * @param stack The stack to search.
 * @param name The name of the symbol to update.
 * @param value The value to assign to the symbol.
 */
void apexSym_setlocal(ScopeStack *stack, const char *name, ApexValue value) {
    LocalScope *scope = stack->top;     
    unsigned int hash = hash_string(name) % scope->table.size;
    Symbol *current = scope->table.symbols[hash];
    while (current) {
        if (current->name == name) {
            ApexValue oldval = current->value;
            apexVal_release(oldval);
            apexVal_retain(value);
            current->value = value;
            return;
        }
        current = current->next;
    }

    Symbol *symbol = apexMem_alloc(sizeof(Symbol));
    symbol->name = name;
    symbol->addr = hash_string(name);
    symbol->value = value;
    symbol->next = scope->table.symbols[hash];
    scope->table.symbols[hash] = symbol;
    scope->table.count++;
}

/**
 * Retrieves the value associated with a given symbol address from the stack.
 *
 * This function walks the stack and looks up the symbol by address. If such a
 * symbol is found, its value is returned. If the symbol is not found, a null
 * value is returned.
 *
 * @param stack The stack to search.
 * @param addr The address of the symbol to look up.
 *
 * @return The value associated with the given symbol address, or a null value
 *         if the symbol is not found.
 */
bool apexSym_getlocal(ApexValue *value, ScopeStack *stack, const char *name) {
    LocalScope *scope = stack->top; 
    unsigned int hash = hash_string(name) % scope->table.size;
    Symbol *current = scope->table.symbols[hash];
    
    while (current) {
        if (current->name == name) {
            *value = current->value;
            return true;
        }
        current = current->next;
    }
    
    return false;
}

/**
 * Looks up a local symbol in the stack and returns its address.
 *
 * This function walks the linked list of local scopes in the stack and
 * looks up the symbol in each one. If the symbol is found, its address is
 * returned. If the symbol is not found in any of the local scopes, -1 is
 * returned.
 *
 * @param stack The stack to search.
 * @param name The name of the local symbol to look up.
 * @return The address of the local symbol if it is found, -1 otherwise.
 */
SymbolAddr get_local_symbol_address(ScopeStack *stack, const char *name) {
    LocalScope *scope = stack->top;
    while (scope != NULL) {
        int addr = get_symbol_address(&scope->table, name);
        if (addr != -1) {
            return addr;
        }
        scope = scope->next;
    }
    return -1;
}

/**
 * Initializes a ScopeStack structure.
 *
 * This function simply sets the top of the stack to NULL, indicating that
 * there are no local scopes currently on the stack.
 *
 * @param stack The ScopeStack structure to initialize.
 */
void init_scope_stack(ScopeStack *stack) {
    stack->top = NULL;
}

/**
 * Frees the memory allocated for a ScopeStack structure.
 *
 * This function iterates over the linked list of local scopes in the stack
 * and frees each one. This function should be called when the ScopeStack
 * structure is no longer needed.
 *
 * @param stack The ScopeStack structure to free.
 */
void free_scope_stack(ScopeStack *stack) {
    while (stack->top) {
        pop_scope(stack);
    }
}