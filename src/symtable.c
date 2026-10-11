#include <stdlib.h>
#include <stdio.h>

#include "symtable.h"


void symtab_init (symtab_t *stab){
    stab->top = NULL;
}

void scope_add (symtab_t *stab){
    symtab_item_t *scope_new = (symtab_item_t *) malloc(sizeof(symtab_item_t));
    if(scope_new == NULL){
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    htab_t *tab;
    htab_init(tab);
    scope_new->scope = tab;
    scope_new->next = stab->top;
    stab->top = scope_new;
}

void scope_rm (symtab_t *stab){
    if(stab->top != NULL){
        symtab_item_t *tmp = stab->top->next;
        free(stab->top);
        stab->top = tmp;
    }
}

void symtab_dtor (symtab_t *stab){
    while(stab->top != NULL){
        scope_rm(stab);
    }
}