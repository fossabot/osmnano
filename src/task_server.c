#include "task_server.h"
#include "fileblock.h"
#include "osm_error.h"

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


int osm_task_server_init(osm_task_server_t *ts) {
    struct sockaddr_un addr;
    socklen_t addrlen = sizeof(struct sockaddr_un);
    int err;

    STAILQ_INIT(&ts->queue);

    ts->sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if(ts->sock == -1) {
        sprintf(osm_error_str, "osm_task_server_init: unable to create socket: %s\n", strerror(errno));
        return ERR_SOCKET;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_NAME);

    err = bind(ts->sock, (const struct sockaddr *)&addr, addrlen);
    if(err != 0) {
        sprintf(osm_error_str, "osm_task_server_init: unable to bind socket: %s\n", strerror(errno));
        return ERR_SOCKET;
    }

    return 0;
}

int osm_task_server_add(osm_task_server_t *ts, osm_task_t *task) {
    STAILQ_INSERT_TAIL(&ts->queue, task, entries);
    return 0;
}

int osm_task_server_get(osm_task_server_t *ts, osm_task_t **task) {
    if(STAILQ_EMPTY(&ts->queue)) {
        return 1;
    }

    *task = STAILQ_FIRST(&ts->queue);
    STAILQ_REMOVE_HEAD(&ts->queue, entries);
    return 0;
}

int osm_task_server_wait(osm_task_server_t *ts) {
    osm_task_t *task;

    while(osm_task_server_get(ts, &task) == 0) {
        osm_fileblock_destroy(&task->fb);
        free(task);
    }
    return 0;
}

void osm_task_server_destroy(osm_task_server_t *ts) {
    close(ts->sock);
    unlink(SOCKET_NAME);
}
