#ifndef FILEBLOCK_H
#define FILEBLOCK_H

#include "fileformat.pb.h"

#include <stdint.h>


struct osm_fileblock_s {
    uint32_t header_size;
    uint32_t header_alloc;
    uint8_t *header;
    OSMPBF_BlobHeader blob_header;

    uint32_t data_size;
    uint8_t *data;
};
typedef struct osm_fileblock_s osm_fileblock_t;

enum {
    OK = 0,
    ERR_READ_HEADER_SIZE,
    ERR_MALLOC,
    ERR_READ_HEADER_DATA,
    ERR_PB_DECODE_HEADER,
    ERR_READ_BLOB_DATA
} osm_fileblock_error;


int osm_fileblock_init(osm_fileblock_t *fb);
int osm_fileblock_read(osm_fileblock_t *fb, int fd);
void osm_fileblock_destroy(osm_fileblock_t *fb);
char *osm_get_error(void);

#endif
