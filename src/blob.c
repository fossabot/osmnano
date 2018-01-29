#include "blob.h"
#include "task_worker.h"
#include "osm_error.h"
#include "fileblock.h"
#include "primitive_block.h"
#include "fileformat.pb.h"
#include "osmformat.pb.h"

#include <pb_decode.h>
#include <miniz.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>



/*
bool osm_blob_header_block(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    OSMPBF_HeaderBlock header_block = OSMPBF_HeaderBlock_init_default;
    bool ok;

    printf("header_block\n");

    ok = pb_decode(stream, OSMPBF_HeaderBlock_fields, &header_block);
    return ok;
}
*/

bool osm_blob_bzip2(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    sprintf(osm_error_str, "osm_blob_unsupported: bzip2 compression is unsupported");
    return false;
}

bool osm_blob_lzma(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    sprintf(osm_error_str, "osm_blob_unsupported: lzma compression is unsupported");
    return false;
}

bool osm_blob_raw(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    osm_blob_t *blob = (osm_blob_t *)arg[0];
    bool ret;

    if(strcmp(blob->fb->blob_header.type, "OSMHeader") == 0) {
        // We don't care about any of the content in a header block right now,
        // so just skip decoding it.
        //ret = osm_blob_header_block(stream, field, arg);
        pb_read(stream, NULL, stream->bytes_left);
        ret = true;
    }else if(strcmp(blob->fb->blob_header.type, "OSMData") == 0) {
        ret = osm_primitive_block(stream, field, arg);
    }else{
        pb_read(stream, NULL, stream->bytes_left);
        sprintf(osm_error_str, "osm_blob_raw: unknown block type: %s", blob->fb->blob_header.type);
        ret = false;
    }
    return ret;
}

bool osm_blob_zlib(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    osm_blob_t *blob = (osm_blob_t *)arg[0];
    pb_istream_t dstream;

    mz_ulong dsize = blob->pb->raw_size;
    uint8_t *dbuf;
    int ret;

    dbuf = malloc(dsize);
    if(dbuf == NULL) {
        sprintf(osm_error_str, "osm_blob_zlib: malloc failed: %s", strerror(errno));
        return false;
    }

    ret = mz_uncompress(dbuf, &dsize, stream->state, stream->bytes_left);
    if(ret != MZ_OK) {
        sprintf(osm_error_str, "osm_blob_zlib: uncompress failed: %s", mz_error(ret));
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

int osm_blob_read_mmap(osm_task_worker_t *worker, osm_blob_t *blob, osm_fileblock_t *fb, int fd) {
    OSMPBF_Blob pb = OSMPBF_Blob_init_default;
    pb_istream_t istream;
    bool ok;

    if(worker->mapped == NULL) {
        worker->mapped = (uint8_t *)mmap(NULL, fb->data_size, PROT_READ, MAP_SHARED, fd, 0);
        if(worker->mapped == MAP_FAILED) {
            worker->mapped = NULL;
            sprintf(osm_error_str, "osm_blob_read_mmap: mmap failed: %s", strerror(errno));
            return ERR_MMAP;
        }
        worker->mapped_size = fb->data_size;
    }

    istream = pb_istream_from_buffer(worker->mapped + fb->data_offset, fb->data_size);

    pb.raw.funcs.decode = &osm_blob_raw;
    pb.raw.arg = blob;
    pb.zlib_data.funcs.decode = &osm_blob_zlib;
    pb.zlib_data.arg = blob;
    pb.lzma_data.funcs.decode = &osm_blob_lzma;
    pb.lzma_data.arg = blob;

    pb.OBSOLETE_bzip2_data.funcs.decode = &osm_blob_bzip2;
    pb.OBSOLETE_bzip2_data.arg = blob;

    blob->fb = fb;
    blob->pb = &pb;

    ok = pb_decode(&istream, OSMPBF_Blob_fields, &pb);
    if(!ok) {
        fprintf(stderr, "%s\n", osm_error_str);
        sprintf(osm_error_str, "osm_blob_read_mmap: pb_decode failed: %s", PB_GET_ERROR(&istream));
        return ERR_READ_BLOB_DATA;
    }

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
    pb.raw.arg = blob;
    pb.zlib_data.funcs.decode = &osm_blob_zlib;
    pb.zlib_data.arg = blob;
    pb.lzma_data.funcs.decode = &osm_blob_lzma;
    pb.lzma_data.arg = blob;

    pb.OBSOLETE_bzip2_data.funcs.decode = &osm_blob_bzip2;
    pb.OBSOLETE_bzip2_data.arg = blob;

    blob->fb = fb;
    blob->pb = &pb;

    ok = pb_decode(&istream, OSMPBF_Blob_fields, &pb);
    if(!ok) {
        free(buf);
        sprintf(osm_error_str, "osm_blob_read: pb_decode failed: %s", PB_GET_ERROR(&istream));
        return ERR_READ_BLOB_DATA;
    }

    free(buf);

    return 0;
}
