#include "ptrlist.h"
#include <inttypes.h>
#include <stdio.h>


int ptrlist_init(ptrlist_t *li, size_t num) {
    li->vals = calloc(num, sizeof(intptr_t) * num);
    if(li->vals == NULL) {
        return 1;
    }
    li->count = 0;
    li->max_size = num;
    return 0;
}

void ptrlist_destroy(ptrlist_t *li) {
    size_t i;
    for(i = 0; i < li->count; i++) {
        free(li->vals[i]);
    }
    li->count = 0;
    li->max_size = 0;
    free(li->vals);
}

int ptrlist_extend(ptrlist_t *li, size_t num) {
    void *newvals;
    if(li->vals == NULL) {
        fprintf(stderr, "Attempt to extend uninitialized ptrlist\n");
        return 1;
    }
    newvals = realloc(li->vals, (sizeof(intptr_t) * num));
    if(newvals == NULL) {
        return 1;
    }else{
        li->vals = newvals;
        li->max_size = num;
        return 0;
    }
}

int ptrlist_append(ptrlist_t *li, void *val) {
    if(li->count >= li->max_size) {
        if(ptrlist_extend(li, (li->max_size * 2)) != 0) {
            return 1;
        }
    }
    li->vals[li->count] = val;
    li->count++;
    return 0;
}
