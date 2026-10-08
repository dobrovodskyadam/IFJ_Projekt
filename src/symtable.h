#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"


typedef struct htab htab_t;

typedef struct symtab_item{
    htab_t *scope;
    symtab_item_t *next;
}symtab_item_t;

typedef struct symtab{
    symtab_item_t *top;
}symtab_t;


void symtab_init(symtab_t *stab);

htab_t *scope_add(symtab_t *stab);
htab_t *scope_rm(symtab_t *stab);

void symtab_dtor(symtab_t *stab);


// TODO - lookup_item();
//        mod_item();
//        struct htab_item;
//        ...


#endif