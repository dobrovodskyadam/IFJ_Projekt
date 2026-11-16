/// @file symtable.c
/// @brief Module for user to interact with table of symbols.
///        Structures themselves are encapsuled and are interacted with only through functions.
/// @date 19-10-2026
///
/// @author Tomas Gruber (xgrubet01)
/// @author Email: xgrubet01@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology

#include <stdio.h>
#include <stdlib.h>

#include "hashtable.h"
#include "symtable.h"
#include "ast.h"

/// @brief Item of stack, acts as a single scope
/// @param scope A hashtable acting like a scope
/// @param next Pointer to next item
typedef struct symtab_item {
    htab_t scope;
    struct symtab_item *next;
} symtab_item_t;

/// @brief Stack structure of table of symbols,
///        simulates layering of scopes
/// @param top Pointer to top of stack, current scope
/// @param id_cnt Counter of added symbols in all scopes
/// @param depth Current depth of stack, -1 == No scope,
///              0 == Only global, 1 == Global + 1 scope, ...
struct symtab {
    symtab_item_t *top;
    unsigned int id_cnt;
    int depth;
};

/// @brief Initialization of table of symbols,
///        use scope_add function before first usage
/// @return Pointer to table of symbols
symtab_t *symtab_init(void) {
    symtab_t *stab = (symtab_t *)malloc(sizeof(symtab_t));
    if (stab == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    stab->top = NULL;
    stab->id_cnt = 0;
    stab->depth = -1; // -1 == contains no scope
    return stab;
}

/// @brief Adds new scope with no symbols
/// @param stab Pointer to table of symbols
void scope_add(symtab_t *stab) {
    symtab_item_t *scope_new = (symtab_item_t *)malloc(sizeof(symtab_item_t));
    if (scope_new == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    htab_t tab = htab_init();
    scope_new->scope = tab;
    scope_new->next = stab->top;
    stab->top = scope_new;
    stab->depth++;
}

/// @brief Removes uppermost scope
/// @param stab Pointer to table of symbols
void scope_rm(symtab_t *stab) {
    symtab_item_t *tmp;

    if (stab->top != NULL) {
        tmp = stab->top->next;
        htab_dtor(stab->top->scope);
        free(stab->top);
        stab->top = tmp;
        stab->depth--;
    }
}

/// @brief Adds a symbol to uppermost scope
/// @param stab Pointer to table of symbols
/// @param id Identifier of symbol
/// @param type Typo of symbol
/// @param modif 1 == modifiable, 0 == not
/// @return True for successfully added item
bool item_add(symtab_t *stab, char *id, Node *decl) {
    if (stab == NULL || stab->top == NULL)
        return false;

    if (hash_item_add(stab->top->scope, id, decl, stab->id_cnt) == false)
        return false;

    stab->id_cnt++;
    return true;
}

/// @brief Searches uppermost scope for symbol
/// @param stab Pointer to table of symbols
/// @param id Identifier of symbol
/// @return True if found
bool item_lookup(symtab_t *stab, char *id) {
    if (stab == NULL)
        return false;

    symtab_item_t *item = stab->top;
    while (item != NULL) {
        if (hash_item_lookup(item->scope, id) != NULL)
            return true;
        item = item->next;
    }
    return false;
}

/// @brief Deletes symbol from uppermost scope
/// @param stab Pointer to table of symbols
/// @param id Identifier of symbol
void item_del(symtab_t *stab, char *id) {
    if (stab == NULL)
        return;

    hash_item_del(stab->top->scope, id);
}

/// @brief Searches all scopes for symbol
/// @param stab Pointer to table of symbols
/// @param id Identifier of symbol
/// @return Item_t structure, if item_t.scope == -1, symbol not found
item_t get_item(symtab_t *stab, char *id) {
    item_t item = {-1, 0, false};
    htab_item_t *hash_item;

    if (item_lookup(stab, id) == false) {
        return item;
    }

    symtab_item_t *scope = stab->top;
    item.scope = stab->depth;
    while ((hash_item = hash_item_lookup(scope->scope, id)) == NULL) {
        item.scope--;
        scope = scope->next;
    }

    item.id_cnt = hash_item->id_cnt;
    item.decl = hash_item->decl;
    return item;
}

/// @brief Destruction of table of symbols
/// @param stab Pointer to table of symbols
void symtab_dtor(symtab_t *stab) {
    if (stab == NULL)
        return;

    while (stab->top != NULL) {
        scope_rm(stab);
    }
    free(stab);
}
