#include <stdlib.h>
#include <stdio.h>

#include "symtable.h"
#include "hashtable.h"

struct symtab_item{
    htab_t scope;
    struct symtab_item *next;
};

typedef struct symtab{
    symtab_item_t *top;
    unsigned int id_cnt;
} symtab_t;

symtab_t *symtab_init (void){
    symtab_t *stab = (symtab_t *) malloc(sizeof(symtab_t));
    if(stab == NULL){
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    stab->top = NULL;
    stab->id_cnt = 0;
    return stab;
}

void scope_add (symtab_t *stab){
    symtab_item_t *scope_new = (symtab_item_t *) malloc(sizeof(symtab_item_t));
    if(scope_new == NULL){
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    htab_t tab = htab_init();
    scope_new->scope = tab;
    scope_new->next = stab->top;
    stab->top = scope_new;
}

void scope_rm (symtab_t *stab){
    symtab_item_t *tmp;

    if(stab->top != NULL){
        tmp = stab->top->next;
        htab_dtor(stab->top->scope);
        free(stab->top);
        stab->top = tmp;
    }
}

void symtab_dtor (symtab_t *stab){
    if(stab == NULL) return;

    while(stab->top != NULL){
        scope_rm(stab);
    }
    free(stab);
}