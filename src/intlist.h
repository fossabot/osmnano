#ifndef INTLIST_H
#define INTLIST_H

#include <stdlib.h>
#include <stdint.h>

struct intlist_s {
    int64_t *vals;
    size_t count;
    size_t max_size;
};
typedef struct intlist_s intlist_t;


int intlist_init(intlist_t *li, size_t num);
void intlist_destroy(intlist_t *li);
int intlist_extend(intlist_t *li, size_t num);
int intlist_append_int64(intlist_t *li, int64_t val);
int intlist_append_uint32(intlist_t *li, uint32_t val);

#endif
