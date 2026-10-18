#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>

#define MAX_ITEM_ID 1024
#define MAX_HTAB_ITEMS 1024

typedef enum {
    VAR,
    FUN,
    INT,
    DUB
} item_type;

typedef enum {
    NOT,
    USE,
    DEL
} item_used;

typedef struct htab_item htab_item_t;

typedef htab_item_t *htab_t;


unsigned hash_func (const char *id);

htab_t htab_init (void);

bool item_add (htab_t tab, char *id, item_type type, bool modif, int id_cnt);
htab_item_t *item_lookup (htab_t tab, char *id);
void item_del (htab_t tab, char *id);


void htab_dtor (htab_t tab);


#endif