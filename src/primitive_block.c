#include "primitive_block.h"
#include "osmformat.pb.h"
#include "osm_error.h"

#include <stdio.h>
#include <stdbool.h>
#include <pb_decode.h>

struct osm_node_s {
    int64_t id;

    // microdegrees 0.000000001
    int64_t lat;
    int64_t lon;
};
typedef struct osm_node_s osm_node_t;


bool osm_primitive_group(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_PrimitiveGroup primitive_group = OSMPBF_PrimitiveGroup_init_default;
    OSMPBF_PrimitiveBlock *primitive_block = (OSMPBF_PrimitiveBlock *)arg[0];
    osm_node_t delta, node;
    uint32_t i;
    bool ok;

    ok = pb_decode(stream, OSMPBF_PrimitiveGroup_fields, &primitive_group);
    if(!ok) {
        sprintf(osm_error_str, "osm_primitive_group: decoding primitive group failed: %s", PB_GET_ERROR(stream));
        return ok;
    }

    //printf("%i dense ids in primitive group\n", primitive_group.dense.id_count);
    if((primitive_group.dense.id_count != primitive_group.dense.lat_count) ||
            (primitive_group.dense.id_count != primitive_group.dense.lon_count)) {
        sprintf(osm_error_str, "osm_primitive_group: mismatched id/lat/lon count");
        return false;
    }

    delta.id = 0;
    delta.lat = 0;
    delta.lon = 0;

    for(i = 0; i < primitive_group.dense.id_count; i++) {
        // delta coding
        delta.id += primitive_group.dense.id[i];
        delta.lat += primitive_group.dense.lat[i];
        delta.lon += primitive_group.dense.lon[i];

        node.id = delta.id;
        node.lat = (primitive_block->lat_offset + (primitive_block->granularity * delta.lat));
        node.lon = (primitive_block->lon_offset + (primitive_block->granularity * delta.lon));
    }

    pb_release(OSMPBF_PrimitiveGroup_fields, &primitive_group);

    return ok;
}

bool osm_primitive_block(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_PrimitiveBlock primitive_block = OSMPBF_PrimitiveBlock_init_default;
    bool ok;

    primitive_block.primitivegroup.funcs.decode = &osm_primitive_group;
    primitive_block.primitivegroup.arg = &primitive_block;

    ok = pb_decode(stream, OSMPBF_PrimitiveBlock_fields, &primitive_block);

    return ok;
}
