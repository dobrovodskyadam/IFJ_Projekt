/// @file hashtable.c
/// @brief Module to interact with hashtable, internal module of symtable.c.
///        Structures themselves are encapsuled and are interacted with only through functions.
/// @date 19-10-2026
///
/// @author Tomas Gruber (xgrubet01)
/// @author Email: xgrubet01@stud.fit.vut.cz
/// Faculty of Information Technology, Brno University of Technology

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "hashtable.h"

#define HTAB_INIT_CAPACITY 101
#define HTAB_MAX_LOAD 0.7

#define BITS_PER_ULONG (sizeof(unsigned long) * CHAR_BIT)

static unsigned long *sieve = NULL;
static unsigned sieve_size = 0;
static unsigned htab_count = 0;

/// @param NOT Never used item
/// @param USE Item currently in use
/// @param DEL Preveously used, now deleted
typedef enum { NOT, USE, DEL } item_used;

/// @brief Item of hashtable, a single symbol
/// @param id     Name of the symbol, points into the AST
/// @param decl   Declaration node (FUNCTION_DEF, PARAMETER or VARIABLE_DECLARATION)
/// @param id_cnt State of id_cnt from symtable in moment of adding (unique number)
/// @param used   NOT == never used, USE == used, DEL == deleted
struct htab_item {
    const char *id;
    struct Node *decl;
    unsigned id_cnt;
    item_used used;
};

/// @brief Hashtable representing single scope
/// @param cap   Capacity of hashtable, can be dynamically increased
/// @param size  Current number of items
/// @param items Array of hashtable items
struct htab {
    unsigned cap;
    unsigned size;
    htab_item_t *items;
};

/// @brief Search in bit array sieve
/// @param idx Index of searched bit
/// @return Bool value of searched bit
static bool sieve_get(unsigned idx){
    return sieve[idx / BITS_PER_ULONG] >> (idx % BITS_PER_ULONG) & 1UL;
}

/// @brief Setting of bit in bit array sieve to true
/// @param idx Index of bit being set
static void sieve_set(unsigned idx){
    unsigned long mask = 1UL << (idx % BITS_PER_ULONG);
    sieve[idx / BITS_PER_ULONG] = sieve[idx / BITS_PER_ULONG] | mask;
}

/// @brief Resizing bit array sieve to new size and calculating new primes
/// @param new_size New size of bit array sieve
static void sieve_resize(unsigned new_size){
    if(new_size <= sieve_size) return;
    unsigned new_size_blocks = (new_size / BITS_PER_ULONG ) + (new_size % BITS_PER_ULONG != 0);
    unsigned old_size_blocks = sieve_size / BITS_PER_ULONG;

    unsigned long *tmp = realloc(sieve, new_size_blocks * sizeof(unsigned long));
    if (tmp == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    memset(tmp + old_size_blocks, 0, 
            (new_size_blocks - old_size_blocks) * sizeof(unsigned long));

    unsigned old_size = sieve_size;
    sieve_size = new_size_blocks * BITS_PER_ULONG;
    sieve = tmp;

    if(old_size_blocks == 0){
        sieve_set(0);
        sieve_set(1);
    }

    for(unsigned idx = (old_size == 0 ? 4 : old_size); idx < sieve_size; idx += 2)
        sieve_set(idx);

    for(unsigned prime = 3; prime <= (sieve_size - 1) / prime; prime += 2){
        if(!sieve_get(prime)){
            unsigned start = prime * prime;
            if(start < old_size) start = old_size;
            while(start % prime != 0) start ++;
            if(start % 2 == 0) start += prime;

            for(unsigned idx = start; idx < sieve_size; idx += 2 * prime)
                sieve_set(idx);
        }
    }

}

/// @brief Searching sieve for prime number bigger than min
/// @param min Minimum size of prime number
/// @return prime number
static unsigned next_prime(size_t min){
    if(min <= 2) return 2;
    if(min % 2 == 0) min ++;
    while(true){
        if(min >= sieve_size){
            sieve_resize(sieve_size == 0 ? HTAB_INIT_CAPACITY : sieve_size * 2);
            continue;
        } 
        if(!sieve_get(min)) return min;
        min += 2;
    }
}

/// @brief Standart hash function, creates hash from id
/// @param id Identifier of symbol
/// @return Created hash
static unsigned hash_func(const char *id) {
    unsigned hash = 0;
    size_t len = strlen(id);
    for (size_t idx = 0; idx < len; idx++) {
        hash = (unsigned char)id[idx] + hash * 65599;
    }
    return hash;
}

/// @brief Resizing and rehousing of hashtable to double its size
/// @param tab Hashtable to be resized
static void htab_resize(htab_t tab){
    unsigned new_cap = next_prime(tab->cap * 2);
    htab_item_t *new_items = calloc(new_cap, sizeof(htab_item_t));
    if(new_items == NULL){
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    unsigned hash;
    for(unsigned idx = 0; idx < tab->cap; idx ++){
        if(tab->items[idx].used == USE){
            hash = hash_func(tab->items[idx].id) % new_cap;
            while(new_items[hash].used == USE) hash = (hash + 1) % new_cap;
            new_items[hash] = tab->items[idx];
        }
    }
    free(tab->items);
    tab->items = new_items;
    tab->cap = new_cap;
}

/// @brief Initialization of hashtable
/// @return Pointer to hashtable
htab_t htab_init(void) {
    htab_t tab = (htab_t)malloc(sizeof(*tab));
    if (tab == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    tab->items = calloc(HTAB_INIT_CAPACITY, sizeof(htab_item_t));
    if (tab->items == NULL) {
        free(tab);
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    tab->cap = HTAB_INIT_CAPACITY;
    tab->size = 0;
    htab_count ++;
    return tab;
}

/// @brief Adds item to hashtable
/// @param tab Hashtable
/// @param id Identifier of symbol
/// @param decl Declaration node of the symbol
/// @param id_cnt Current id_cnt
/// @return True if successfully added
bool hash_item_add(htab_t tab, char *id, struct Node *decl, unsigned id_cnt) {
    if (tab == NULL || tab->items == NULL || id == NULL || hash_item_lookup(tab, id) != NULL)
        return false;

    if((double)(tab->size + 1) / tab->cap >= HTAB_MAX_LOAD) 
        htab_resize(tab);

    unsigned hash = hash_func(id) % tab->cap;

    while (tab->items[hash].used == USE)
        hash = hash + 1 % tab->cap;

    unsigned idx = hash;

    // Store the pointer
    tab->items[idx].id = id;
    tab->items[idx].decl = decl;
    tab->items[idx].id_cnt = id_cnt;
    tab->items[idx].used = USE;

    tab->size ++;
    return true;
}

/// @brief Searches symtable for id
/// @param tab Hashtable
/// @param id Identifier of symbol
/// @return Pointer to searched item
htab_item_t *hash_item_lookup(htab_t tab, char *id) {
    if(tab == NULL || id == NULL) return NULL;
    htab_item_t *item = NULL;
    unsigned hash = hash_func(id) % tab->cap;
    int cnt = 0;
    while (cnt < tab->cap &&
           (tab->items[hash].used == USE || tab->items[hash].used == DEL)) {
        if (tab->items[hash].used == USE && !strcmp(id, tab->items[hash].id)) {
            item = &tab->items[hash];
            break;
        }
        hash = (hash + 1) % tab->cap;
        cnt++;
    }
    return item;
}

/// @brief Deletes item
/// @param tab Hashtable
/// @param id Identifier of symbol
void hash_item_del(htab_t tab, char *id) {
    htab_item_t *item = hash_item_lookup(tab, id);
    if (item != NULL && item->used == USE){
        item->used = DEL;
        tab->size --;
    }
}

/// @brief Destruction of hashtable
/// @param tab Hashtable
void htab_dtor(htab_t tab) {
    if(tab == NULL) return;
    free(tab->items);
    free(tab);

    htab_count --;
    if(htab_count <= 0){
        free(sieve);
        sieve = NULL;
        sieve_size = 0;
    }
}

/// @brief
struct Node *hash_item_decl(const htab_item_t *item) {
    return item->decl;
}

/// @brief
unsigned hash_item_id_cnt(const htab_item_t *item) {
    return item->id_cnt;
}
