#include "osm_error.h"
#include "fileblock.h"
#include "blob.h"
#include "task_server.h"

#include "osmnano.pb.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


/*
int worker(void) {
    osm_blob_t blob;
    err = osm_blob_init(&blob);
    if(err != 0) {
        fprintf(stderr, "%s\n", osm_get_error());
        return err;
    }
    fd = open(argv[1], O_RDONLY);
    if(fd == -1) {
        osm_fileblock_destroy(&fb);
        fprintf(stderr, "file open failed: %s\n", strerror(errno));
        return 1;
    }

    err = osm_fileblock_seek_begin(&fb, fd);
    if(err != 0) {
        break;
    }
    err = osm_blob_read(&blob, &fb, fd);

    return err;
}
*/


int main(int argc, char **argv) {
    osm_task_server_t task_server;
    osm_task_t *task;

    off_t file_offset, progress_interval, next_progress;
    int fd;
    int err = 0;
    int num_blocks = 0;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    err = osm_task_server_init(&task_server);
    if(err != 0) {
        fprintf(stderr, "task server init failed: %s\n", osm_get_error());
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if(fd == -1) {
        fprintf(stderr, "file open failed: %s\n", strerror(errno));
        return 1;
    }

    file_offset = lseek(fd, 0, SEEK_END);
    if(file_offset == -1) {
        fprintf(stderr, "unable to determine file size: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    printf("File size: %lu (%lu GB)\n", file_offset, (file_offset / (1024 * 1024 * 1024)));


    progress_interval = (file_offset / 10);
    next_progress = progress_interval;

    file_offset = lseek(fd, 0, SEEK_SET);
    if(file_offset == -1) {
        fprintf(stderr, "seek to beginning of file: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    setbuf(stdout, NULL);
    printf("Reading block headers");

    while(err == OK) {
        task = malloc(sizeof(osm_task_t));
        if(task == NULL) {
            sprintf(osm_error_str, "unable to allocate memory for task: %s", strerror(errno));
            err = ERR_MALLOC;
            break;
        }

        err = osm_fileblock_init(&task->fb);
        if(err != OK) {
            free(task);
            break;
        }

        err = osm_fileblock_read(&task->fb, fd);
        if(err != OK) {
            free(task);
            break;
        }

        if(task->fb.data_offset >= next_progress) {
            printf(".");
            next_progress += progress_interval;
        }

        err = osm_fileblock_seek_end(&task->fb, fd);
        if(err != OK) {
            break;
        }

        err = osm_task_server_add(&task_server, task);
        if(err != OK) {
            break;
        }

        num_blocks++;
    }

    printf(". %d blocks\n", num_blocks);

    if(err != ERR_EOF && err != OK) {
        fprintf(stderr, "%s\n", osm_get_error());
    }

    close(fd);

    osm_task_server_wait(&task_server);
    osm_task_server_destroy(&task_server);

    return err;
}
