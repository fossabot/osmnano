#ifndef OSMFORMAT_H
#define OSMFORMAT_H

#include <pb.h>
#include <stdbool.h>

bool osm_unpack_keys_vals(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_unpack_sint64(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_primitivegroup(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool osm_stringtable(pb_istream_t *stream, const pb_field_t *field, void **arg);

#endif
