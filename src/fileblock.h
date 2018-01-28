#ifndef FILEBLOCK_H
#define FILEBLOCK_H

#include "fileformat.pb.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>


struct osm_fileblock_s {
    uint32_t header_size;
    size_t header_alloc;
    uint8_t *header;
    OSMPBF_BlobHeader blob_header;

    uint32_t data_size;
    off_t data_offset;

    char filename[1024];
};
typedef struct osm_fileblock_s osm_fileblock_t;

int osm_fileblock_init(osm_fileblock_t *fb, char *filename);
int osm_fileblock_read(osm_fileblock_t *fb, int fd);
int osm_fileblock_seek_begin(osm_fileblock_t *fb, int fd);
int osm_fileblock_seek_end(osm_fileblock_t *fb, int fd);
void osm_fileblock_destroy(osm_fileblock_t *fb);

#endif
