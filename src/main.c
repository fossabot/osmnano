#include "osm_error.h"
#include "fileblock.h"
#include "blob.h"
#include "task_server.h"
#include "task_worker.h"

#include "osmnano.pb.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


int main(int argc, char **argv) {
    osm_task_server_t task_server;
    osm_task_t *task;

    off_t file_offset, progress_interval, next_progress;
    char *filename;
    int fd;
    int err = 0;
    int num_blocks = 0;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    err = osm_task_server_init(&task_server);
    if(err != 0) {
        fprintf(stderr, "task server init failed: %s\n", osm_get_error());
        return 1;
    }

    for(int i = 0; i < 8; i++) {
        err = osm_task_worker_fork(&task_server);
        if(err == ERR_NO_TASKS) {
            osm_task_server_destroy(&task_server);
            return 0;
        }
        if(err != 0) {
            osm_task_server_destroy(&task_server);
            fprintf(stderr, "task worker failed: %s\n", osm_get_error());
            return 1;
        }
    }

    fd = open(filename, O_RDONLY);
    if(fd == -1) {
        osm_task_server_destroy(&task_server);
        fprintf(stderr, "file open failed: %s\n", strerror(errno));
        return 1;
    }

    file_offset = lseek(fd, 0, SEEK_END);
    if(file_offset == -1) {
        osm_task_server_destroy(&task_server);
        fprintf(stderr, "unable to determine file size: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    printf("File size: %lu (%lu GB)\n", file_offset, (file_offset / (1024 * 1024 * 1024)));


    progress_interval = (file_offset / 10);
    next_progress = progress_interval;

    file_offset = lseek(fd, 0, SEEK_SET);
    if(file_offset == -1) {
        osm_task_server_destroy(&task_server);
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

        task->id = num_blocks + 1;

        err = osm_fileblock_init(&task->fb, filename);
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

        err = osm_task_server_add(&task_server, task);
        if(err != OK) {
            break;
        }

        err = osm_task_server_loop(&task_server, false);
        if(err != OK) {
            break;
        }

        err = osm_fileblock_seek_end(&task->fb, fd);
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

    printf("Executing tasks");
    progress_interval = (num_blocks / 10);
    next_progress = progress_interval;

    while(osm_task_server_inflight(&task_server) > 0) {
        if(task_server.completed_tasks >= next_progress) {
            printf(".");
            next_progress += progress_interval;
        }
        err = osm_task_server_loop(&task_server, true);
        if(err != OK) {
            fprintf(stderr, "%s\n", osm_get_error());
            break;
        }
    }

    printf(". %lu complete\n", task_server.completed_tasks);

    err = osm_task_server_wait(&task_server);
    if(err != 0 && err != ERR_NO_TASKS) {
        fprintf(stderr, "%s\n", osm_get_error());
        return err;
    }

    osm_task_server_destroy(&task_server);

    return err;
}
