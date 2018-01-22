#ifndef FILEFORMAT_H
#define FILEFORMAT_H

#include "fileformat.pb.h"
#include "osmformat.pb.h"

#include "ptrlist.h"
#include "intlist.h"
#include <stdbool.h>
#include <stdint.h>
#include <pb.h>

#define MAX_HEADER_SIZE 65536


enum next_blob {
    HEADER_BLOCK = 0,
    PRIMITIVE_BLOCK
};

struct decode_state_s {
    enum next_blob next_blob_type;
    OSMPBF_Blob *blob;
    OSMPBF_PrimitiveBlock *primitive_block;
    ptrlist_t stringtable;
};
typedef struct decode_state_s decode_state_t;

struct dense_s {
    intlist_t ids;
    intlist_t lats;
    intlist_t lons;
};
typedef struct dense_s dense_t;


int osm_decode_size(int fd, uint32_t *dest);
int osm_decode_header(int fd, OSMPBF_BlobHeader *header, uint32_t header_size, decode_state_t *state);
int osm_decode_blob(int fd, OSMPBF_Blob *blob, uint32_t blob_size, decode_state_t *state);
bool osm_blob_raw(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_blob_zlib(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_blob_lzma(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_blob_bzip2(pb_istream_t *stream, const pb_field_t *field, void **arg);

#endif
