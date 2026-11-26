/// @file hashtable.c
/// @brief Module to interact with hashtable, internal module of symtable.c.
///        Structures themselves are encapsuled and are interacted with only through functions.
/// @date 19-10-2026
///
/// @author Tomas Gruber (xgrubet01)
/// @author Email: xgrubet01@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

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

/// @brief Standart hash function, creates hash from id
/// @param id Identifier of symbol
/// @return Created hash
unsigned hash_func(const char *id) {
    unsigned hash = 0;
    size_t len = strlen(id);
    for (size_t idx = 0; idx < len; idx++) {
        hash = (unsigned char)id[idx] + hash * 65599;
    }
    return hash;
}

/// @brief Initialization of hashtable
/// @return Pointer to hashtable
htab_t htab_init(void) {
    htab_t tab = (htab_t)calloc(MAX_HTAB_ITEMS, sizeof(htab_item_t));
    if (tab == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    return tab;
}

/// @brief Adds item to hashtable
/// @param tab Hashtable
/// @param id Identifier of symbol
/// @param decl Declaration node of the symbol
/// @param id_cnt Current id_cnt
/// @return True if successfully added
bool hash_item_add(htab_t tab, char *id, struct Node *decl, int id_cnt) {
    if (tab == NULL || id == NULL || hash_item_lookup(tab, id) != NULL)
        return false;

    unsigned hash = hash_func(id);
    int cnt = 0;

    while (cnt < MAX_HTAB_ITEMS && tab[hash % MAX_HTAB_ITEMS].used == USE) {
        hash++;
        cnt++;
    }

    if (cnt == MAX_HTAB_ITEMS)
        return false;
    unsigned idx = hash % MAX_HTAB_ITEMS;

    // Store the pointer
    tab[idx].id = id;
    tab[idx].decl = decl;
    tab[idx].id_cnt = id_cnt;
    tab[idx].used = USE;
    return true;
}

/// @brief Searches symtable for id
/// @param tab Hashtable
/// @param id Identifier of symbol
/// @return Pointer to searched item
htab_item_t *hash_item_lookup(htab_t tab, char *id) {
    htab_item_t *item = NULL;
    unsigned hash = hash_func(id);
    int cnt = 0;
    while (cnt < MAX_HTAB_ITEMS &&
           (tab[hash % MAX_HTAB_ITEMS].used == USE || tab[hash % MAX_HTAB_ITEMS].used == DEL)) {
        if (tab[hash % MAX_HTAB_ITEMS].used == USE && !strcmp(id, tab[hash % MAX_HTAB_ITEMS].id)) {
            item = &tab[hash % MAX_HTAB_ITEMS];
            break;
        }
        hash++;
        cnt++;
    }
    return item;
}

/// @brief Deletes item
/// @param tab Hashtable
/// @param id Identifier of symbol
void hash_item_del(htab_t tab, char *id) {
    htab_item_t *item = hash_item_lookup(tab, id);
    if (item != NULL && item->used == USE)
        item->used = DEL;
}

/// @brief Destruction of hashtable
/// @param tab Hashtable
void htab_dtor(htab_t tab) {
    free(tab);
}

/// @brief
struct Node *hash_item_decl(const htab_item_t *item) {
    return item->decl;
}

/// @brief
unsigned hash_item_id_cnt(const htab_item_t *item) {
    return item->id_cnt;
}
