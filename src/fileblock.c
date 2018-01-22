#include "fileblock.h"
#include "osmformat.pb.h"
#include <pb_decode.h>
#include <miniz.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>


char osm_error_str[1024] = "OK";

char *osm_get_error(void) {
    return osm_error_str;
}

int osm_read_header_size(int fd, uint32_t *dest) {
    uint32_t size;
    if(read(fd, &size, sizeof(uint32_t)) < 1) {
        strcpy(osm_error_str, strerror(errno));
        return ERR_READ_HEADER_SIZE;
    }

    *dest = ntohl(size);
    return 0;
}

int osm_parse_header(osm_fileblock_t *fb) {
    OSMPBF_BlobHeader blob_header = OSMPBF_BlobHeader_init_default;
    pb_istream_t istream;
    bool ok;

    istream = pb_istream_from_buffer(fb->header, fb->header_size);
    ok = pb_decode(&istream, OSMPBF_BlobHeader_fields, &blob_header);
    if(!ok) {
        strcpy(osm_error_str, PB_GET_ERROR(&istream));
        return ERR_PB_DECODE_HEADER;
    }

    fb->blob_header = blob_header;

    return 0;
}

int osm_fileblock_read(osm_fileblock_t *fb, int fd) {
    uint32_t offset = 0;
    int err;

    fb->header_size = 0;
    fb->data_size = 0;

    err = osm_read_header_size(fd, &fb->header_size);
    if(err != 0) {
        return err;
    }

    fb->header = malloc(fb->header_size);
    if(fb->header == NULL) {
        fb->header_size = 0;
        strcpy(osm_error_str, "Unable to allocate memory for header");
        return ERR_MALLOC;
    }
    memset(fb->header, 0, fb->header_size);

    while(offset < fb->header_size) {
        err = read(fd, fb->header + offset, (fb->header_size - offset));
        if(err == -1) {
            fb->header_size = 0;
            free(fb->header);
            strcpy(osm_error_str, strerror(errno));
            return ERR_READ_HEADER_DATA;
        }
        if(err == 0) {
            strcpy(osm_error_str, "EOF when reading header size");
            return ERR_EOF;
        }
    }

    err = osm_parse_header(fb);
    if(err != 0) {
        fb->header_size = 0;
        free(fb->header);
        return err;
    }

    fb->data_size = fb->blob_header.datasize;
    fb->data = malloc(fb->data_size);
    if(fb->data == NULL) {
        fb->data_size = 0;
        strcpy(osm_error_str, "Unable to allocate memory for blob data");
        return ERR_MALLOC;
    }
    memset(fb->header, 0, fb->header_size);

    offset = 0;
    while(offset < fb->data_size) {
        err = read(fd, fb->data + offset, (fb->data_size - offset));
        if(err == -1) {
            fb->data_size = 0;
            free(fb->data);
            strcpy(osm_error_str, strerror(errno));
            return ERR_READ_BLOB_DATA;
        }
        if(err == 0) {
            break;
        }
    }

    return 0;
}

void osm_fileblock_destroy(osm_fileblock_t *fb) {
    if(fb->header_size != 0) {
#ifdef PB_ENABLE_MALLOC
        pb_release(OSMPBF_BlobHeader_fields, fb->blob_header);
#endif
        free(fb->header);
    }

    if(fb->data_size != 0) {
        free(fb->data);
    }
}
