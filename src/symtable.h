#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"


typedef htab_item_t *htab;

struct symtab_item{
    htab_t *scope;
    struct symtab_item *next;
};

typedef struct symtab_item symtab_item_t;

typedef struct symtab{
    symtab_item_t *top;
    unsigned int id_cnt;
} symtab_t;


void symtab_init (symtab_t *stab);

void scope_add (symtab_t *stab);
void scope_rm (symtab_t *stab);

void symtab_dtor (symtab_t *stab);

#endif