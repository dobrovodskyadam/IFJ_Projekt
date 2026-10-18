#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"


typedef struct symtab_item symtab_item_t;

typedef struct symtab symtab_t;


symtab_t *symtab_init (void);

void scope_add (symtab_t *stab);
void scope_rm (symtab_t *stab);

void symtab_dtor (symtab_t *stab);

#endif