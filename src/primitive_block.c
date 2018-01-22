#include "primitive_block.h"
#include "osmformat.pb.h"

#include <stdio.h>
#include <stdbool.h>
#include <pb_decode.h>


bool osm_primitive_group(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_PrimitiveGroup primitive_group = OSMPBF_PrimitiveGroup_init_default;
    bool ok;

    ok = pb_decode(stream, OSMPBF_PrimitiveGroup_fields, &primitive_group);
    return ok;
}

bool osm_primitive_block(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_PrimitiveBlock primitive_block = OSMPBF_PrimitiveBlock_init_default;
    bool ok;

    primitive_block.primitivegroup.funcs.decode = &osm_primitive_group;
    primitive_block.primitivegroup.arg = arg;

    ok = pb_decode(stream, OSMPBF_PrimitiveBlock_fields, &primitive_block);

    return ok;
}
