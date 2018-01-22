#include "fileformat.h"
#include "osmformat.h"
#include <pb_decode.h>
#include <miniz.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include "intlist.h"
#include "ptrlist.h"


int osm_decode_size(int fd, uint32_t *dest) {
    uint32_t size;
    if(read(fd, &size, sizeof(uint32_t)) < 1) {
        return 1;
    }

    *dest = ntohl(size);
    return 0;
}

int osm_decode_header(int fd, OSMPBF_BlobHeader *header, uint32_t header_size, decode_state_t *state) {
    uint8_t header_buf[MAX_HEADER_SIZE];
    pb_istream_t istream;
    int size;
    int offset = 0;
    bool ok;

    while(header_size > 0) {
        size = read(fd, header_buf + offset, (header_size - offset));
        if(size < 0) {
            fprintf(stderr, "Unable to read header: %s\n", strerror(errno));
            header = NULL;
            return 1;
        }
        if(size == 0) {
            fprintf(stderr, "Hit EOF when reading header\n");
            header = NULL;
            return 1;
        }

        header_size -= size;
        offset += size;
    }

    istream = pb_istream_from_buffer(header_buf, offset);

    ok = pb_decode(&istream, OSMPBF_BlobHeader_fields, header);
    if(!ok) {
        fprintf(stderr, "Decode header failed: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }

    if(strcmp(header->type, "OSMHeader") == 0) {
        state->next_blob_type = HEADER_BLOCK;
    }else if(strcmp(header->type, "OSMData") == 0) {
        state->next_blob_type = PRIMITIVE_BLOCK;
    }else{
        fprintf(stderr, "Unknown header type: %s\n", header->type);
        return 1;
    }

#ifdef PB_ENABLE_MALLOC
    pb_release(OSMPBF_BlobHeader_fields, header);
#endif
    return 0;
}

int osm_decode_blob(int fd, OSMPBF_Blob *blob, uint32_t blob_size, decode_state_t *state) {
    uint8_t *buf;
    pb_istream_t istream;
    int size;
    int offset = 0;
    bool ok;

    state->blob = blob;

    buf = malloc(blob_size);
    if(buf == NULL) {
        fprintf(stderr, "Unable to allocate %d bytes for blob\n", blob_size);
        return 1;
    }

    while(blob_size > 0) {
        size = read(fd, buf + offset, (blob_size - offset));
        if(size < 0) {
            fprintf(stderr, "Unable to read blob: %s\n", strerror(errno));
            blob = NULL;
            free(buf);
            return 1;
        }
        if(size == 0) {
            fprintf(stderr, "Hit EOF when reading blob\n");
            blob = NULL;
            free(buf);
            return 1;
        }

        blob_size -= size;
        offset += size;
    }

    istream = pb_istream_from_buffer(buf, offset);

    ok = pb_decode(&istream, OSMPBF_Blob_fields, blob);
    if(!ok) {
        fprintf(stderr, "Decode blob failed: %s\n", PB_GET_ERROR(&istream));
        free(buf);
        return 1;
    }

#ifdef PB_ENABLE_MALLOC
    pb_release(OSMPBF_Blob_fields, blob);
#endif
    free(buf);
    return 0;
}

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
