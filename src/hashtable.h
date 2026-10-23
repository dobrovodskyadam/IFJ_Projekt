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

#define MAX_ITEM_ID 1024
#define MAX_HTAB_ITEMS 1031 // Maximum of hashtable items, should be prime number

/// @param VAR Variable
/// @param FUN Function
/// @param INT Integer
/// @param DUB Double
typedef enum {
    VAR,
    FUN,
    INT,
    DUB
} item_type;

/// @param NOT Never used item
/// @param USE Item currently in use
/// @param DEL Preveously used, now deleted
typedef enum {
    NOT,
    USE, 
    DEL
} item_used;

typedef struct htab_item htab_item_t;

typedef htab_item_t *htab_t;


unsigned hash_func (const char *id);

htab_t htab_init (void);

bool hash_item_add (htab_t tab, char *id, item_type type, bool modif, int id_cnt);
htab_item_t *hash_item_lookup (htab_t tab, char *id);
void hash_item_del (htab_t tab, char *id);


void htab_dtor (htab_t tab);


#endif