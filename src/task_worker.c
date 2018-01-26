#include "task_worker.h"
#include "osm_error.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>


int osm_task_worker_fork(struct addrinfo *addr) {
    osm_task_worker_t worker;
    osm_task_t task;
    pid_t pid;
    int err;

    pid = fork();
    if(pid == 0) {
        // child process
        err = osm_task_worker_connect(&worker, addr);
        if(err != 0) {
            sprintf(osm_error_str, "osm_task_worker_fork: failed to connect to parent: %s", strerror(errno));
        }

        while(osm_task_worker_connected(&worker)) {
            err = osm_task_worker_get_task(&worker, &task);
            if(err != 0) {
                break;
            }

            err = osm_task_worker_run(&worker, &task);
            if(err != 0) {
                break;
            }

            err = osm_task_worker_finish(&worker, &task);
            if(err != 0) {
                break;
            }
        }

        osm_task_worker_destroy(&worker);

        return err;
    }else{
        printf("Task worker pid %d\n", pid);
        // parent process
    }

    return 0;
}

int osm_task_worker_connect(osm_task_worker_t *worker, struct addrinfo *addr) {
    int err;

    worker->sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(worker->sock == -1) {
        sprintf(osm_error_str, "osm_task_worker_connect: unable to create socket: %s", strerror(errno));
        return ERR_SOCKET;
    }

    err = connect(worker->sock, addr->ai_addr, addr->ai_addrlen);
    if(err != 0) {
        close(worker->sock);
        sprintf(osm_error_str, "osm_task_worker_connect: failed to connect to task server: %s", strerror(errno));
        return ERR_SOCKET;
    }

    return 0;
}

bool osm_task_worker_connected(osm_task_worker_t *worker) {
    struct sockaddr_in6 addr;
    socklen_t addrlen = sizeof(struct sockaddr_in6);
    int err;

    err = getpeername(worker->sock, (struct sockaddr *)&addr, &addrlen);
    if(err != 0) {
        return false;
    }else{
        return true;
    }
}

int osm_task_worker_get_task(osm_task_worker_t *worker, osm_task_t *task) {
    sprintf(osm_error_str, "osm_task_worker_get_task: no more tasks available");
    return ERR_NO_TASKS;
}

int osm_task_worker_run(osm_task_worker_t *worker, osm_task_t *task) {
    return 0;
}

int osm_task_worker_finish(osm_task_worker_t *worker, osm_task_t *task) {
    return 0;
}

void osm_task_worker_destroy(osm_task_worker_t *worker) {
    if(osm_task_worker_connected(worker)) {
        close(worker->sock);
    }
}
