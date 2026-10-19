#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"


typedef struct symtab_item symtab_item_t;

typedef struct symtab symtab_t;

typedef struct item{
    int scope;
    item_type type;
    bool modif;
}item_t;

symtab_t *symtab_init (void);


void scope_add (symtab_t *stab);
void scope_rm (symtab_t *stab);

bool item_add (symtab_t *stab, char *id, item_type type, bool modif);
bool item_lookup (symtab_t *stab, char *id);
void item_del (symtab_t *stab, char *id);

item_t get_item(symtab_t *stab, char *id);

void symtab_dtor (symtab_t *stab);

#endif