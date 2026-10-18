#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hashtable.h"

struct htab_item{
    char id[MAX_ITEM_ID];
    item_type type;
    bool modif;
    unsigned id_cnt;
    item_used used;
};

unsigned hash_func (const char *id){
    unsigned hash = 0;
    size_t len = strlen(id);
    for(size_t idx = 0; idx < len; idx ++){
        hash = (unsigned char)id[idx] + hash * 65599;
    }
    return hash;
}

htab_t htab_init (void){
    htab_t tab = (htab_t)calloc(MAX_HTAB_ITEMS, sizeof(htab_item_t));
    if(tab == NULL){
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    return tab;
}

bool hash_item_add (htab_t tab, char *id, item_type type, bool modif, int id_cnt){
    if(tab == NULL || id == NULL ||
        strlen(id) >= MAX_ITEM_ID || 
        hash_item_lookup(tab, id) != NULL) return false;
    
    unsigned hash = hash_func(id);
    int cnt = 0;
    
    while(cnt < MAX_HTAB_ITEMS && tab[hash % MAX_HTAB_ITEMS].used == USE){ 
        hash ++;
        cnt ++;
    }

    if(cnt == MAX_HTAB_ITEMS) return false;
    unsigned idx = hash % MAX_HTAB_ITEMS;

    strcpy(tab[idx].id, id);
    tab[idx].type = type;
    tab[idx].modif = modif;
    tab[idx].id_cnt = id_cnt;
    tab[idx].used = USE;
    
    return true;
}

htab_item_t *hash_item_lookup(htab_t tab, char *id){
    htab_item_t *item = NULL;
    unsigned hash = hash_func(id);
    int cnt = 0;
    while(cnt < MAX_HTAB_ITEMS && 
        (tab[hash % MAX_HTAB_ITEMS].used == USE || tab[hash % MAX_HTAB_ITEMS].used == DEL)){
        if(tab[hash % MAX_HTAB_ITEMS].used == USE && !strcmp(id, tab[hash % MAX_HTAB_ITEMS].id)){
            item = &tab[hash % MAX_HTAB_ITEMS];
            break;
        }
        hash ++;
        cnt ++;
    }
    return item;
}

void hash_item_del (htab_t tab, char *id){
    htab_item_t* item = hash_item_lookup(tab, id);
    if(item != NULL && item->used == USE)
        item->used = DEL;
}

void htab_dtor (htab_t tab){
    free(tab);
}

