bool osm_blob_raw(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_HeaderBlock header_block = OSMPBF_HeaderBlock_init_default;
    OSMPBF_PrimitiveBlock primitive_block = OSMPBF_PrimitiveBlock_init_default;
    decode_state_t *state = (decode_state_t *)arg[0];
    bool ok;

    state->primitive_block = &primitive_block;

    switch(state->next_blob_type) {
        case HEADER_BLOCK:
            ok = pb_decode(stream, OSMPBF_HeaderBlock_fields, &header_block);
#ifdef PB_ENABLE_MALLOC
            pb_release(OSMPBF_HeaderBlock_fields, &header_block);
#endif
            break;
        case PRIMITIVE_BLOCK:
            /*
            primitive_block.stringtable.s.funcs.decode = &osm_stringtable;
            primitive_block.stringtable.s.arg = state;
            */

            primitive_block.primitivegroup.funcs.decode = &osm_primitivegroup;
            primitive_block.primitivegroup.arg = state;
            ok = pb_decode(stream, OSMPBF_PrimitiveBlock_fields, &primitive_block);
#ifdef PB_ENABLE_MALLOC
            pb_release(OSMPBF_PrimitiveBlock_fields, &primitive_block);
#endif
            break;
        default:
            printf("unknown blob type\n");
            ok = false;
    }

    if(!ok) {
        fprintf(stderr, "Failed to decode block type %d\n", state->next_blob_type);
    }

    return true;
}

bool osm_blob_zlib(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    decode_state_t *state = (decode_state_t *)arg[0];
    mz_ulong dsize = state->blob->raw_size;
    pb_istream_t dstream;
    uint8_t *dbuf;
    int ret;
    bool result;

    dbuf = malloc(dsize);
    if(dbuf == NULL) {
        fprintf(stderr, "Unable to allocate memory for dbuf\n");
        return false;
    }

    // Uncompress directly from the stream buffer, don't copy it first.
    if((ret = mz_uncompress(dbuf, &dsize, stream->state, stream->bytes_left)) != MZ_OK) {
        fprintf(stderr, "uncompress failed: %d\n", ret);
        free(dbuf);
        return false;
    }

    // Seek forward to consume the rest of the stream
    pb_read(stream, NULL, stream->bytes_left);

    dstream = pb_istream_from_buffer(dbuf, dsize);

    result = osm_blob_raw(&dstream, field, arg);

    //free(zbuf);
    free(dbuf);
    return result;
}

bool osm_blob_lzma(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    printf("LZMA compression not supported\n");
    return false;
}

bool osm_blob_bzip2(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    printf("BZIP2 compression not supported\n");
    return false;
}
