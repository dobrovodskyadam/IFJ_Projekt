/// @file symtable.h
/// @brief Header file for module symtable.c, table of symbols.
///        Structures themselves are encapsuled and are interacted with only through functions.
/// @date 19-10-2026
///
/// @author Tomas Gruber (xgrubet01)
/// @author Email: xgrubet01@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>

typedef struct symtab_item symtab_item_t;
typedef struct symtab symtab_t;
typedef struct Node Node;

/// @brief Structure to receive values for get_item
/// @param scope  Absolute scope value, -1 == Search failed,
///               0 == Global scope, 1 == First scope above global, ...
/// @param id_cnt Unique number of the symbol (for unique names in code generation)
/// @param decl   Declaration node of the symbol
typedef struct item {
    int scope;
    unsigned id_cnt;
    Node *decl;
} item_t;


symtab_t *symtab_init(void);

void scope_add(symtab_t *stab);
void scope_rm(symtab_t *stab);

bool item_add(symtab_t *stab, char *id, Node *decl);
bool item_lookup(symtab_t *stab, char *id);
void item_del(symtab_t *stab, char *id);

item_t get_item(symtab_t *stab, char *id);

void symtab_dtor(symtab_t *stab);

#endif
