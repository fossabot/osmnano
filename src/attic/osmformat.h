#ifndef OSMFORMAT_H
#define OSMFORMAT_H

#include <pb.h>
#include <stdbool.h>

/*
 * The OSM wiki says a file block should contain around 8000 entries, but some
 * extracts have considerably more nodes per block. At least this works without
 * a realloc on correctly formatted files.
 */
#define LIST_INIT_SIZE 8000

bool osm_unpack_keys_vals(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_unpack_sint64(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_primitivegroup(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_stringtable(pb_istream_t *stream, const pb_field_t *field, void **arg);

#endif
