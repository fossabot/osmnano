#include "blob.h"
#include "osm_error.h"
#include "fileblock.h"
#include "fileformat.pb.h"

#include <pb_decode.h>
#include <miniz.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

bool osm_blob_raw(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    printf("osm_blob_raw\n");
    pb_read(stream, NULL, stream->bytes_left);
    return true;
}

bool osm_blob_zlib(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_Blob *blob = (OSMPBF_Blob *)arg[0];
    pb_istream_t dstream;

    mz_ulong dsize = blob->raw_size;
    uint8_t *dbuf;
    int ret;

    dbuf = malloc(dsize);
    if(dbuf == NULL) {
        sprintf(osm_error_str, "osm_blob_zlib: malloc failed: %s", strerror(errno));
        return false;
    }

    ret = mz_uncompress(dbuf, &dsize, stream->state, stream->bytes_left);
    if(ret != MZ_OK) {
        sprintf(osm_error_str, "osm_blob_zlib: uncompress failed");
        free(dbuf);
        return false;
    }

    pb_read(stream, NULL, stream->bytes_left);

    dstream = pb_istream_from_buffer(dbuf, dsize);
    ret = osm_blob_raw(&dstream, field, arg);
    free(dbuf);

    return ret;
}

int osm_blob_init(osm_blob_t *blob) {
    return 0;
}

int osm_blob_read(osm_blob_t *blob, osm_fileblock_t *fb, int fd) {
    OSMPBF_Blob pb = OSMPBF_Blob_init_default;
    pb_istream_t istream;

    uint8_t *buf;
    uint32_t offset;
    bool ok;
    int err;

    buf = malloc(fb->data_size);
    if(buf == NULL) {
        return ERR_MALLOC;
    }

    offset = 0;
    while(offset < fb->data_size) {
        err = read(fd, (buf + offset), (fb->data_size - offset));
        if(err < 0) {
            free(buf);
            sprintf(osm_error_str, "osm_blob_read: read failed: %s", strerror(errno));
            return ERR_READ_BLOB_DATA;
        }
        if(err == 0) {
            sprintf(osm_error_str, "osm_blob_read: unexpected EOF in the middle of a blob");
            return ERR_EOF;
        }
        offset += err;
    }

    istream = pb_istream_from_buffer(buf, fb->data_size);

    pb.raw.funcs.decode = &osm_blob_raw;
    pb.raw.arg = &blob;
    pb.zlib_data.funcs.decode = &osm_blob_zlib;
    pb.zlib_data.arg = &pb;

    ok = pb_decode(&istream, OSMPBF_Blob_fields, &pb);
    if(!ok) {
        free(buf);
        sprintf(osm_error_str, "osm_blob_read: pb_decode failed: %s", PB_GET_ERROR(&istream));
        return ERR_READ_BLOB_DATA;
    }

    free(buf);

    return 0;
}
