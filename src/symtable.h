#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "hashtable.h"


typedef htab_t;

typedef struct {
    htab_t *scope;
    symtab_item_t *next;
} symtab_item_t;

typedef struct {
    symtab_item_t *top;
    unsigned int id_cnt;
} symtab_t;


void symtab_init (symtab_t *stab);

void scope_add (symtab_t *stab);
void scope_rm (symtab_t *stab);

void symtab_dtor (symtab_t *stab);


// TODO - lookup_item();
//        mod_item();
//        struct htab_item;
//        ...


#endif