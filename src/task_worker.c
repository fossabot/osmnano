#include "task_worker.h"
#include "task_server.h"
#include "task.h"
#include "blob.h"
#include "osm_error.h"
#include "osmnano.pb.h"

#include <pb_decode.h>
#include <pb_encode.h>
#include <pb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>


int osm_task_worker_fork(osm_task_server_t *ts) {
    osm_task_worker_t worker;
    osm_task_t task;
    pid_t pid;
    int err;

    pid = fork();

    /*
     * for profiling, gprof needs this
    extern void _start (void), etext (void);
    monstartup ((u_long) &_start, (u_long) &etext);
    */

    if(pid == 0) {
        // child process
        err = osm_task_worker_connect(&worker, &ts->addrs[0]);
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

        if(err == 0) {
            sprintf(osm_error_str, "osm_task_worker_fork: No more tasks");
            err = ERR_NO_TASKS;
        }

        return err;
    }else{
        // parent process
        ts->pids[ts->num_workers] = pid;
        //printf("Task worker %d pid %d\n", ts->num_workers, pid);
        ts->num_workers++;
        return 0;
    }
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
    uint8_t buf[65536];
    osmnano_Task task_pb;
    pb_istream_t istream;
    ssize_t err;
    bool ok;

    err = recv(worker->sock, buf, 65536, 0);
    if(err == -1) {
        sprintf(osm_error_str, "osm_task_worker_get_task: recv failed: %s", strerror(errno));
        return ERR_SOCKET;
    }

    if(err == 0) {
        sprintf(osm_error_str, "osm_task_worker_get_task: socket closed");
        return ERR_NO_TASKS;
    }

    istream = pb_istream_from_buffer(buf, err);
    ok = pb_decode(&istream, osmnano_Task_fields, &task_pb);
    if(!ok) {
        sprintf(osm_error_str, "osm_task_worker_get_task: unable to decode Task: %s", PB_GET_ERROR(&istream));
        return ERR_PB_DECODE;
    }
    task->id = task_pb.id;
    osm_fileblock_init(&task->fb, task_pb.filename);
    task->fb.data_offset = task_pb.data_offset;
    task->fb.data_size = task_pb.data_size;
    task->fb.blob_header = task_pb.blob_header;

    return 0;
}

int osm_task_worker_run(osm_task_worker_t *worker, osm_task_t *task) {
    osm_blob_t blob;
    int err;
    int fd;

    err = osm_blob_init(&blob);
    if(err != 0) {
        return err;
    }

    fd = open(task->fb.filename, O_RDONLY);
    if(fd == -1) {
        sprintf(osm_error_str, "osm_task_worker_run: open failed: %s", strerror(errno));
        return ERR_SEEK;
    }

    err = osm_fileblock_seek_begin(&task->fb, fd);
    if(err != 0) {
        close(fd);
        return err;
    }

    err = osm_blob_read(&blob, &task->fb, fd);
    if(err != 0) {
        close(fd);
        return err;
    }

    close(fd);
    return 0;
}

int osm_task_worker_finish(osm_task_worker_t *worker, osm_task_t *task) {
    osmnano_TaskStatus task_status;
    pb_ostream_t ostream;
    uint8_t buf[65536];
    bool ok;
    int err;

    osm_fileblock_destroy(&task->fb);

    task_status.id = task->id;
    task_status.status = osmnano_TaskStatus_Status_SUCCESS;

    ostream = pb_ostream_from_buffer(buf, 65536);
    ok = pb_encode(&ostream, osmnano_TaskStatus_fields, &task_status);
    if(!ok) {
        sprintf(osm_error_str, "osm_task_worker_finish: unable to encode TaskStatus: %s", PB_GET_ERROR(&ostream));
        return ERR_PB_ENCODE;
    }

    err = send(worker->sock, buf, ostream.bytes_written, 0);
    if(err == -1) {
        sprintf(osm_error_str, "osm_task_worker_finish: error sending task status: %s", strerror(errno));
        return ERR_SOCKET;
    }
    if(err == 0) {
        sprintf(osm_error_str, "osm_task_worker_finish: sending task status failed, socket closed");
        return ERR_SOCKET;
    }

    return 0;
}

void osm_task_worker_destroy(osm_task_worker_t *worker) {
    if(osm_task_worker_connected(worker)) {
        close(worker->sock);
    }
}
