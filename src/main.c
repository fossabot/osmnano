#include "fileblock.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char **argv) {
    osm_fileblock_t fb;
    int fd;
    int err = 0;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    err = osm_fileblock_init(&fb);
    if(err != 0) {
        printf("%d:%s\n", err, osm_get_error());
        return err;
    }

    fd = open(argv[1], O_RDONLY);

    while(err == 0) {
        err = osm_fileblock_read(&fb, fd);
    }

    if(err != 0) {
        printf("%d:%s\n", err, osm_get_error());
    }

    osm_fileblock_destroy(&fb);
    close(fd);

    return err;
}
