#include "osm_error.h"
#include "fileblock.h"
#include "blob.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char **argv) {
    osm_fileblock_t fb;
    osm_blob_t blob;
    int fd;
    int err = 0;
    int count = 0;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    err = osm_fileblock_init(&fb);
    if(err != 0) {
        fprintf(stderr, "%s\n", osm_get_error());
        return err;
    }

    err = osm_blob_init(&blob);
    if(err != 0) {
        fprintf(stderr, "%s\n", osm_get_error());
        return err;
    }

    fd = open(argv[1], O_RDONLY);

    while(err == OK) {
        err = osm_fileblock_read(&fb, fd);
        if(err != OK) {
            break;
        }

        err = osm_blob_read(&blob, &fb, fd);
        if(err != OK) {
            break;
        }
        count++;
    }

    if(err != ERR_EOF) {
        fprintf(stderr, "%s\n", osm_get_error());
    }


    osm_fileblock_destroy(&fb);
    close(fd);

    printf("Read %d blocks successfully\n", count);
    return err;
}
