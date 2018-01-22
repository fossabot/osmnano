#ifndef BLOB_H
#define BLOB_H

#include "fileblock.h"

struct osm_blob_s {
    osm_fileblock_t *fb;
    OSMPBF_Blob *pb;
};
typedef struct osm_blob_s osm_blob_t;

int osm_blob_init(osm_blob_t *blob);
int osm_blob_read(osm_blob_t *blob, osm_fileblock_t *fb, int fd);

#endif
