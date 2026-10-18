#include <stdlib.h>
#include <stdio.h>

#include "symtable.h"
#include "hashtable.h"

typedef struct symtab_item{
    htab_t scope;
    struct symtab_item *next;
} symtab_item_t;

struct symtab{
    symtab_item_t *top;
    unsigned int id_cnt;
};

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

bool item_add (symtab_t *stab, char *id, item_type type, bool modif){
    if(stab == NULL) return false;

    if(hash_item_add(stab->top->scope, id, type, modif, stab->id_cnt) == false)
        return false;
    return true;
}

bool item_lookup (symtab_t *stab, char *id){
    if(stab == NULL) return false;

    symtab_item_t *item = stab->top;
    while(item != NULL){
        if(hash_item_lookup(item->scope, id) != NULL) return true;
        item = item->next;
    }
    return false;
}

void item_del (symtab_t *stab, char *id){
    if(stab == NULL) return false;

    hash_item_del(stab->top->scope, id);
}

void symtab_dtor (symtab_t *stab){
    if(stab == NULL) return;

    while(stab->top != NULL){
        scope_rm(stab);
    }
    free(stab);
}