#include "blob.h"
#include "osm_error.h"
#include "fileblock.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>


int osm_blob_init(osm_blob_t *blob) {
    return 0;
}

int osm_blob_read(osm_blob_t *blob, osm_fileblock_t *fb, int fd) {
    uint8_t *buf;
    uint32_t offset;
    off_t saved, erro;
    int err;


    saved = lseek(fd, 0, SEEK_CUR);
    if(saved == -1) {
        sprintf(osm_error_str, "osm_blob_read: lseek failed when saving cursor: %s", strerror(errno));
        return ERR_READ_BLOB_DATA;
    }

    erro = lseek(fd, fb->data_offset, SEEK_SET);
    if(erro == -1) {
        sprintf(osm_error_str, "osm_blob_read: lseek to data offset failed: %s", strerror(errno));
        return ERR_READ_BLOB_DATA;
    }

    buf = malloc(fb->data_size);
    if(buf == NULL) {
        return ERR_MALLOC;
    }

    offset = 0;
    while(offset < fb->data_size) {
        err = read(fd, (buf + offset), (fb->data_size - offset));
        if(err == -1) {
            free(buf);
            lseek(fd, saved, SEEK_SET);
            sprintf(osm_error_str, "osm_blob_read: read failed: %s", strerror(errno));
            return ERR_READ_BLOB_DATA;
        }
        if(err == 0) {
            break;
        }
        offset += err;
    }

    erro = lseek(fd, saved, SEEK_SET);
    if(erro == -1) {
        free(buf);
        sprintf(osm_error_str, "osm_blob_read: lseek failed when trying to restore saved cursor: %s", strerror(errno));
        return ERR_READ_BLOB_DATA;
    }

    free(buf);

    return 0;
}
