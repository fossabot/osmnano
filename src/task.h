#ifndef TASK_H
#define TASK_H
#include "fileblock.h"
#include <sys/queue.h>

typedef struct osm_task_s osm_task_t;
struct osm_task_s {
    uint64_t id;
    osm_fileblock_t fb;
    STAILQ_ENTRY(osm_task_s) entries;
};
#endif
