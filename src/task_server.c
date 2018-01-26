#include "task_server.h"
#include "fileblock.h"
#include "osm_error.h"

#include "osmnano.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/queue.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


int osm_task_server_init(osm_task_server_t *ts) {
    struct addrinfo hints;
    int err;

    STAILQ_INIT(&ts->queue);
    SLIST_INIT(&ts->sessions);
    SLIST_INIT(&ts->cleanup);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    err = getaddrinfo("::1", TASK_SERVER_PORT, &hints, &ts->addrs);
    if(err != 0) {
        sprintf(osm_error_str, "osm_task_server_init: unable to determine address: %s", gai_strerror(err));
        return ERR_SOCKET;
    }

    ts->sock = socket(ts->addrs[0].ai_addr->sa_family, ts->addrs[0].ai_socktype, ts->addrs[0].ai_protocol);
    if(ts->sock == -1) {
        sprintf(osm_error_str, "osm_task_server_init: unable to create socket: %s", strerror(errno));
        return ERR_SOCKET;
    }

    err = setsockopt(ts->sock, SOL_SOCKET, SO_REUSEADDR, &((int){1}), sizeof(int));
    if(err == -1) {
        close(ts->sock);
        sprintf(osm_error_str, "osm_task_server_init: unable to set SO_REUSEADDR: %s", strerror(errno));
        return ERR_SOCKET;
    }

    err = bind(ts->sock, ts->addrs[0].ai_addr, ts->addrs[0].ai_addrlen);
    if(err != 0) {
        close(ts->sock);
        sprintf(osm_error_str, "osm_task_server_init: unable to bind socket: %s", strerror(errno));
        return ERR_SOCKET;
    }

    err = listen(ts->sock, 10);
    if(err != 0) {
        close(ts->sock);
        sprintf(osm_error_str, "osm_task_server_init: unable to listen on socket: %s", strerror(errno));
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

int osm_task_server_loop(osm_task_server_t *ts) {
    osm_session_t *session;
    struct timeval timeout;
    fd_set readfds;
    int nfds;
    int err;

    while(!SLIST_EMPTY(&ts->cleanup)) {
        printf("cleanup\n");
        session = SLIST_FIRST(&ts->cleanup);
        SLIST_REMOVE(&ts->sessions, session, osm_session_s, entries);
        SLIST_REMOVE_HEAD(&ts->cleanup, entries);
        close(session->sock);
        free(session);
    }

    if(ts->sock > FD_SETSIZE) {
        sprintf(osm_error_str, "osm_task_server_loop: server socket descriptor %d is greater than FD_SETSIZE=%d", ts->sock, FD_SETSIZE);
        return ERR_SOCKET;
    }
    FD_ZERO(&readfds);
    FD_SET(ts->sock, &readfds);
    nfds = ts->sock + 1;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    err = select(nfds, &readfds, NULL, NULL, &timeout);
    if(err == -1) {
        sprintf(osm_error_str, "osm_task_server_loop: select to accept failed: %s", strerror(errno));
        return ERR_SOCKET;
    }
    if(err > 0) {
        session = malloc(sizeof(osm_session_t));
        if(session == NULL) {
            sprintf(osm_error_str, "osm_task_server_loop: cannot allocate memory for incoming connection: %s", strerror(errno));
            return ERR_MALLOC;
        }
        session->addrlen = sizeof(struct sockaddr);
        session->sock = accept(ts->sock, &session->addr, &session->addrlen);
        if(session->sock == -1) {
            free(session);
            sprintf(osm_error_str, "osm_task_server_loop: accept failed: %s", strerror(errno));
            return ERR_SOCKET;
        }
        SLIST_INSERT_HEAD(&ts->sessions, session, entries);
    }

    FD_ZERO(&readfds);
    nfds = 0;
    SLIST_FOREACH(session, &ts->sessions, entries) {
        FD_SET(session->sock, &readfds);
        if(session->sock > (nfds - 1)) {
            nfds = (session->sock + 1);
        }
    }

    err = select(nfds, &readfds, NULL, NULL, &timeout);
    if(err == -1) {
        sprintf(osm_error_str, "osm_task_server_loop: select to recv failed: %s", strerror(errno));
        return ERR_SOCKET;
    }
    if(err > 0) {
        SLIST_FOREACH(session, &ts->sessions, entries) {
            if(FD_ISSET(session->sock, &readfds)) {
                err = osm_task_server_handle(ts, session);
                if(err != 0) {
                    session->err = err;
                    SLIST_INSERT_HEAD(&ts->cleanup, session, entries);
                    return err;
                }
            }
        }
    }

    return 0;
}

int osm_task_server_handle(osm_task_server_t *ts, osm_session_t *session) {
    osmnano_TaskStatus task_status;
    osmnano_Task task_pb;
    osm_task_t *task;

    pb_istream_t istream;
    pb_ostream_t ostream;

    uint8_t buf[65536];
    ssize_t len;
    bool ok;
    int err;

    len = recv(session->sock, buf, 65536, 0);
    if(len == -1) {
        sprintf(osm_error_str, "osm_task_server_handle: recv failed: %s", strerror(errno));
        return ERR_SOCKET;
    }
    if(len == 0) {
        close(session->sock);
        return 0;
    }

    istream = pb_istream_from_buffer(buf, len);
    ok = pb_decode(&istream, osmnano_TaskStatus_fields, &task_status);
    if(!ok) {
        sprintf(osm_error_str, "osm_task_server_handle: decoding TaskStatus failed: %s", PB_GET_ERROR(&istream));
        return ERR_PB_DECODE;
    }

    switch(task_status.status) {
        case osmnano_TaskStatus_Status_SUCCESS:
            printf("Task %lu success\n", task_status.id);
            break;
        case osmnano_TaskStatus_Status_FAILED:
            printf("Task %lu failed\n", task_status.id);
            break;
        default:
            printf("Task %lu unknown status %d\n", task_status.id, task_status.status);
    }

    err = osm_task_server_get(ts, &task);
    if(err != 0) {
        return err;
    }

    task_pb.id = 0;
    strcpy(task_pb.filename, task->fb.filename);
    task_pb.data_offset = task->fb.data_offset;
    task_pb.blob_header = task->fb.blob_header;

    ostream = pb_ostream_from_buffer(buf, 65536);
    ok = pb_encode(&ostream, osmnano_Task_fields, &task_pb);
    if(!ok) {
        sprintf(osm_error_str, "osm_task_server_handle: encoding Task failed: %s", PB_GET_ERROR(&ostream));
        return ERR_PB_ENCODE;
    }

    return 0;
}

bool osm_task_server_empty(osm_task_server_t *ts) {
    return STAILQ_EMPTY(&ts->queue);
}

int osm_task_server_wait(osm_task_server_t *ts) {
    return 0;
}

void osm_task_server_destroy(osm_task_server_t *ts) {
    osm_session_t *session;
    osm_task_t *task;

    while(osm_task_server_get(ts, &task) == 0) {
        osm_fileblock_destroy(&task->fb);
        free(task);
    }

    while(!SLIST_EMPTY(&ts->sessions)) {
        session = SLIST_FIRST(&ts->sessions);
        SLIST_REMOVE_HEAD(&ts->sessions, entries);
        close(session->sock);
        free(session);
    }

    while(!SLIST_EMPTY(&ts->cleanup)) {
        session = SLIST_FIRST(&ts->cleanup);
        SLIST_REMOVE_HEAD(&ts->cleanup, entries);
        close(session->sock);
        free(session);
    }

    close(ts->sock);
    freeaddrinfo(ts->addrs);
}
