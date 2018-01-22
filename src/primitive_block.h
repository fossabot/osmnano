#ifndef PRIMITIVE_BLOCK_H
#define PRIMITIVE_BLOCK_H

#include <stdbool.h>
#include <pb.h>

bool osm_primitive_block(pb_istream_t *stream, const pb_field_t *field, void **arg);


#endif
