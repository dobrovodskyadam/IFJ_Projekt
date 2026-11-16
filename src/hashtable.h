/// @file hashtable.h
/// @brief Header file for module hashtable.c, scope of table of symbols.
///        Internal header for symtable.c module.
/// @date 19-10-2026
///
/// @author Tomas Gruber (xgrubet01)
/// @author Email: xgrubet01@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

#define MAX_HTAB_ITEMS 101 // Maximum of hashtable items, should be prime number

struct Node;

/// @param NOT Never used item
/// @param USE Item currently in use
/// @param DEL Preveously used, now deleted
typedef enum { NOT, USE, DEL } item_used;

/// @brief Item of hashtable, a single symbol
/// @param id     Name of the symbol, points into the AST
/// @param decl   Declaration node (FUNCTION_DEF, PARAMETER or VARIABLE_DECLARATION)
/// @param id_cnt State of id_cnt from symtable in moment of adding (unique number)
/// @param used   NOT == never used, USE == used, DEL == deleted
typedef struct htab_item {
    const char *id;
    struct Node *decl;
    unsigned id_cnt;
    item_used used;
} htab_item_t;

typedef htab_item_t *htab_t;


unsigned hash_func(const char *id);

htab_t htab_init(void);

bool hash_item_add(htab_t tab, char *id, struct Node *decl, int id_cnt);
htab_item_t *hash_item_lookup(htab_t tab, char *id);
void hash_item_del(htab_t tab, char *id);


void htab_dtor(htab_t tab);


#endif
