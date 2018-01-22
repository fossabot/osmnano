#ifndef PTRLIST_H
#define PTRLIST_H

#include <stdlib.h>

struct ptrlist_s {
    void **vals;
    size_t count;
    size_t max_size;
};
typedef struct ptrlist_s ptrlist_t;


int ptrlist_init(ptrlist_t *li, size_t num);
void ptrlist_destroy(ptrlist_t *li);
int ptrlist_extend(ptrlist_t *li, size_t num);
int ptrlist_append(ptrlist_t *li, void *val);

#endif
