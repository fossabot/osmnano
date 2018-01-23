#include "osm_error.h"
#include "fileblock.h"
#include "blob.h"

#include "osmnano.pb.h"

#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define NUM_WORKERS 12


struct osm_task_s {
    osm_fileblock_t fb;
    TAILQ_ENTRY(osm_task_s) node;
};
typedef struct osm_task_s osm_task_t;

TAILQ_HEAD(osm_task_q, osm_task_s) tasks;

void start_workers(int num_workers) {
    printf("Starting %d worker threads\n", num_workers);
}
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
    TAILQ_INIT(&tasks);
    osm_task_t *task;

    off_t file_offset, progress_interval, next_progress;
    int fd;
    int err = 0;
    int num_blocks = 0;
    int blocks_processed = 0;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    start_workers(NUM_WORKERS);

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

        TAILQ_INSERT_TAIL(&tasks, task, node);

        err = osm_fileblock_seek_end(&task->fb, fd);
        if(err != OK) {
            break;
        }

        num_blocks++;
    }

    printf(". %d blocks\n", num_blocks);

    printf("Parsing blocks");
    
    progress_interval = (num_blocks / 10);
    next_progress = 0;

    while(!TAILQ_EMPTY(&tasks)) {
        task = TAILQ_FIRST(&tasks);
        //printf("task.fb.data_offset=%lu\n", task->fb.data_offset);
        TAILQ_REMOVE(&tasks, task, node);

        osm_fileblock_destroy(&task->fb);
        free(task);

        blocks_processed++;
        if(blocks_processed > next_progress) {
            printf(".");
            next_progress += progress_interval;
        }
    }

    printf(". OK\n");

    if(err != ERR_EOF && err != OK) {
        fprintf(stderr, "%s\n", osm_get_error());
    }

    close(fd);


    return err;
}
