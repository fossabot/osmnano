#include "fileblock.h"
#include "osm_error.h"
#include "osmformat.pb.h"
#include <pb_decode.h>
#include <miniz.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


int osm_read_header_size(int fd, uint32_t *dest) {
    uint32_t size;
    int err;

    err = read(fd, &size, sizeof(uint32_t));
    if(err == -1) {
        sprintf(osm_error_str, "osm_read_header_size: %s", strerror(errno));
        return ERR_READ_HEADER_SIZE;
    }
    if(err == 0) {
        return ERR_EOF;
    }

    *dest = ntohl(size);
    return 0;
}

int osm_parse_header(osm_fileblock_t *fb) {
    pb_istream_t istream;
    bool ok;

    istream = pb_istream_from_buffer(fb->header, fb->header_size);
    ok = pb_decode(&istream, OSMPBF_BlobHeader_fields, &fb->blob_header);
    if(!ok) {
        sprintf(osm_error_str, "osm_parse_header: %s", PB_GET_ERROR(&istream));
        return ERR_PB_DECODE_HEADER;
    }

    return 0;
}

int osm_fileblock_init(osm_fileblock_t *fb) {
    OSMPBF_BlobHeader blob_header = OSMPBF_BlobHeader_init_default;
    fb->blob_header = blob_header;

    fb->header_size = 0;
    fb->header_alloc = 0;
    fb->header = NULL;

    fb->data_size = 0;
    fb->data_offset = 0;
    return 0;
}

int osm_fileblock_read(osm_fileblock_t *fb, int fd) {
    uint32_t offset = 0;
    uint32_t new_size;
    uint8_t *tmp;
    int err;

    err = osm_read_header_size(fd, &fb->header_size);
    if(err != 0) {
        return err;
    }

    new_size = (fb->header_size + 1);

    if(new_size > fb->header_alloc) {
        tmp = realloc(fb->header, new_size);
        if(tmp == NULL) {
            fb->header_size = 0;
            sprintf(osm_error_str, "osm_fileblock_read: Unable to allocate memory for header");
            return ERR_MALLOC;
        }else{
            fb->header_alloc = new_size;
            fb->header = tmp;
            fb->header[fb->header_size] = '\0';
        }
    }

    while(offset < fb->header_size) {
        err = read(fd, fb->header + offset, (fb->header_size - offset));
        if(err == -1) {
            fb->header_size = 0;
            sprintf(osm_error_str, "osm_fileblock_read: failed to read header: %s", strerror(errno));
            return ERR_READ_HEADER_DATA;
        }
        if(err == 0) {
            break;
        }
        offset += err;
    }

    err = osm_parse_header(fb);
    if(err != 0) {
        fb->header_size = 0;
        return err;
    }

    fb->data_size = fb->blob_header.datasize;

    err = lseek(fd, 0, SEEK_CUR);
    if(err == -1) {
        fb->data_size = 0;
        sprintf(osm_error_str, "osm_fileblock_read: lseek failed: %s", strerror(errno));
        return ERR_READ_BLOB_DATA;
    }
    fb->data_offset = err;

    return 0;
}

void osm_fileblock_destroy(osm_fileblock_t *fb) {
    if(fb->header_alloc != 0) {
        free(fb->header);
        fb->header_alloc = 0;
    }
}
