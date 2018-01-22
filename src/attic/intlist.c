#include "intlist.h"


int intlist_init(intlist_t *li, size_t num) {
    li->vals = calloc(num, sizeof(int64_t));
    if(li->vals == NULL) {
        return 1;
    }
    li->count = 0;
    li->max_size = num;
    return 0;
}

void intlist_destroy(intlist_t *li) {
    li->count = 0;
    li->max_size = 0;
    free(li->vals);
}

int intlist_extend(intlist_t *li, size_t num) {
    void *newvals;
    newvals = realloc(li->vals, (sizeof(int64_t) * num));
    if(newvals == NULL) {
        return 1;
    }else{
        li->vals = newvals;
        li->max_size = num;
        return 0;
    }
}

int intlist_append_int64(intlist_t *li, int64_t val) {
    if(li->count >= li->max_size) {
        if(intlist_extend(li, (li->max_size * 2)) != 0) {
            return 1;
        }
    }
    li->vals[li->count] = val;
    li->count++;
    return 0;
}

int intlist_append_uint32(intlist_t *li, uint32_t val) {
    if(li->count >= li->max_size) {
        if(intlist_extend(li, (li->max_size * 2)) != 0) {
            return 1;
        }
    }
    li->vals[li->count] = (int64_t)val;
    li->count++;
    return 0;
}
