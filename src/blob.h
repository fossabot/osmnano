#ifndef BLOB_H
#define BLOB_H

#include "fileblock.h"
#include "task_worker.h"

struct osm_blob_s {
    osm_fileblock_t *fb;
    OSMPBF_Blob *pb;
};
typedef struct osm_blob_s osm_blob_t;

int osm_blob_init(osm_blob_t *blob);
int osm_blob_read(osm_blob_t *blob, osm_fileblock_t *fb, int fd);
int osm_blob_read_mmap(osm_task_worker_t *worker, osm_blob_t *blob, osm_fileblock_t *fb, int fd);

#endif
