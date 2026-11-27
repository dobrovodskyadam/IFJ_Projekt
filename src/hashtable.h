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

struct Node;

typedef struct htab_item htab_item_t;
typedef struct htab *htab_t;

htab_t htab_init(void);

bool hash_item_add(htab_t tab, char *id, struct Node *decl, unsigned id_cnt);
htab_item_t *hash_item_lookup(htab_t tab, char *id);
void hash_item_del(htab_t tab, char *id);

struct Node *hash_item_decl(const htab_item_t *item);
unsigned hash_item_id_cnt(const htab_item_t *item);

void htab_dtor(htab_t tab);


#endif
