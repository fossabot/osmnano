#include "fileformat.h"
#include "osmformat.h"
#include <pb_decode.h>
#include <pb.h>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>


bool osm_unpack_keys_vals(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    //decode_state_t *state = (decode_state_t *)arg[0];
    int32_t keyid = 0;
    int32_t stringid = 0;
    size_t nodeid = 0;

    //uint8_t *key, *val;

    while(stream->bytes_left > 0) {
        if(!pb_decode_varint32(stream, (uint32_t *)&keyid)) {
            return false;
        }
        if(keyid == 0) {
            nodeid++;
            continue;
        }
        if(!pb_decode_varint32(stream, (uint32_t *)&stringid)) {
            return false;
        }

        /*
        if(state->stringtable.count >= keyid && state->stringtable.count >= stringid) {
            key = state->stringtable.vals[keyid];
            val = state->stringtable.vals[stringid];
            printf("node=%zu %s=%s\n", nodeid, key, val);
        }
        */
    }

    return true;
}

bool osm_unpack_sint64(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    int64_t last_val = 0;
    int64_t delta = 0;
    int64_t val = 0;

    intlist_t *list = (intlist_t *)arg[0];

    while(stream->bytes_left > 0) {
        if(!pb_decode_svarint(stream, &delta)) {
            return false;
        }
        val = last_val + delta;
        last_val = val;
        if(intlist_append_int64(list, val) != 0) {
            return false;
        }
    }

    return true;
}

bool osm_primitivegroup(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_PrimitiveGroup primitive_group = OSMPBF_PrimitiveGroup_init_default;
    decode_state_t *state = (decode_state_t *)arg[0];
    bool ok;
    int i, err;
    float lat, lon;

    dense_t dense;


    err = intlist_init(&dense.ids, LIST_INIT_SIZE);
    if(err != 0) {
        return false;
    }

    err = intlist_init(&dense.lats, LIST_INIT_SIZE);
    if(err != 0) {
        intlist_destroy(&dense.ids);
        return false;
    }

    err = intlist_init(&dense.lons, LIST_INIT_SIZE);
    if(err != 0) {
        intlist_destroy(&dense.ids);
        intlist_destroy(&dense.lats);
        return false;
    }

    primitive_group.dense.id.funcs.decode = &osm_unpack_sint64;
    primitive_group.dense.id.arg = &dense.ids;

    primitive_group.dense.lat.funcs.decode = &osm_unpack_sint64;
    primitive_group.dense.lat.arg = &dense.lats;

    primitive_group.dense.lon.funcs.decode = &osm_unpack_sint64;
    primitive_group.dense.lon.arg = &dense.lons;

    //primitive_group.dense.keys_vals.funcs.decode = &osm_unpack_keys_vals;
    //primitive_group.dense.keys_vals.arg = state;

    ok = pb_decode(stream, OSMPBF_PrimitiveGroup_fields, &primitive_group);

    /*
    for(i = 0; i < dense.ids.count; i++) {
        lat = 0.000000001 * (state->primitive_block->lat_offset + (state->primitive_block->granularity * dense.lats.vals[i]));
        lon = 0.000000001 * (state->primitive_block->lon_offset + (state->primitive_block->granularity * dense.lons.vals[i]));

        printf("id=% " PRId64 " lat=%f lon=%f\n",
               dense.ids.vals[i], lat, lon);
    }
    */

    intlist_destroy(&dense.ids);
    intlist_destroy(&dense.lats);
    intlist_destroy(&dense.lons);

#ifdef PB_ENABLE_MALLOC
    pb_release(OSMPBF_PrimitiveGroup_fields, &primitive_group);
#endif
    return ok;
}

bool osm_stringtable(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    decode_state_t *state = (decode_state_t *)arg[0];
    uint8_t *buf;
    size_t len = stream->bytes_left;
    bool ok;

    buf = malloc(len + 1);
    if(buf == NULL) {
        return false;
    }
    buf[len] = '\0';

    ok = pb_read(stream, buf, len);

    if(ptrlist_append(&state->stringtable, (void *)buf) != 0) {
        free(buf);
        return false;
    }

    return ok;
}
