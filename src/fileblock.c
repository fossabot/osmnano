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
    pb_istream_t istream;
    bool ok;

    istream = pb_istream_from_buffer(fb->header, fb->header_size);
    ok = pb_decode(&istream, OSMPBF_BlobHeader_fields, &fb->blob_header);
    if(!ok) {
        strcpy(osm_error_str, PB_GET_ERROR(&istream));
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
            strcpy(osm_error_str, "Unable to allocate memory for header");
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
            strcpy(osm_error_str, strerror(errno));
            return ERR_READ_HEADER_DATA;
        }
        if(err == 0) {
            fb->header_size = 0;
            strcpy(osm_error_str, "EOF when reading header size");
            return OK;
        }
    }

    err = osm_parse_header(fb);
    if(err != 0) {
        fb->header_size = 0;
        return err;
    }

    fb->data_size = fb->blob_header.datasize;

    /*
    fb->data = malloc(fb->data_size + 1);
    fb->data[fb->data_size] = '\0';
    if(fb->data == NULL) {
        fb->data_size = 0;
        strcpy(osm_error_str, "Unable to allocate memory for blob data");
        return ERR_MALLOC;
    }
    */

    err = lseek(fd, fb->data_size, SEEK_CUR);
    if(err == -1) {
        fb->data_size = 0;
        strcpy(osm_error_str, strerror(errno));
        return ERR_READ_BLOB_DATA;
    }
    fb->data_offset = (err - fb->data_size);
    /*
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
    */

    return 0;
}

void osm_fileblock_destroy(osm_fileblock_t *fb) {
    if(fb->header_alloc != 0) {
        free(fb->header);
    }

    /*
    if(fb->data_size != 0) {
        free(fb->data);
    }
    */
}