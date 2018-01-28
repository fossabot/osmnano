#include "task_server.h"
#include "fileblock.h"
#include "osm_error.h"

#include "osmnano.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
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

    memset(ts->pids, 0, sizeof(pid_t) * MAX_WORKERS);
    ts->num_workers = 0;
    ts->completed_tasks = 0;

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
    ts->submitted_tasks++;
    return 0;
}

int osm_task_server_get(osm_task_server_t *ts, osm_task_t **task) {
    if(STAILQ_EMPTY(&ts->queue)) {
        return ERR_NO_TASKS;
    }

    *task = STAILQ_FIRST(&ts->queue);
    STAILQ_REMOVE_HEAD(&ts->queue, entries);
    return 0;
}

int osm_task_server_accept(osm_task_server_t *ts) {
    osm_session_t *session;
    struct timeval timeout;
    fd_set readfds;
    int nfds;
    int err;

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
        if(errno == EINTR) {
            return 0;
        }
        sprintf(osm_error_str, "osm_task_server_loop: select to accept failed: %s", strerror(errno));
        return ERR_SOCKET;
    }

    if(err > 0) {
        session = malloc(sizeof(osm_session_t));
        if(session == NULL) {
            sprintf(osm_error_str, "osm_task_server_loop: cannot allocate memory for incoming connection: %s", strerror(errno));
            return ERR_MALLOC;
        }
        err = osm_session_init(session);
        if(err != 0) {
            free(session);
            return err;
        }

        session->sock = accept(ts->sock, NULL, NULL);
        if(session->sock == -1) {
            osm_session_destroy(session);
            free(session);
            sprintf(osm_error_str, "osm_task_server_loop: accept failed: %s", strerror(errno));
            return ERR_SOCKET;
        }
        SLIST_INSERT_HEAD(&ts->sessions, session, entries);
    }

    return 0;
}

int osm_task_server_recv(osm_task_server_t *ts, bool wait) {
    osm_session_t *session;
    struct timeval timeout;
    fd_set readfds;
    int nfds = 0;
    int err;

    if(!wait) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    }else{
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
    }

    FD_ZERO(&readfds);
    SLIST_FOREACH(session, &ts->sessions, entries) {
        FD_SET(session->sock, &readfds);
        if(session->sock > (nfds - 1)) {
            nfds = (session->sock + 1);
        }
    }

    if(nfds > 0) {
        err = select(nfds, &readfds, NULL, NULL, &timeout);
        if(err == -1) {
            if(errno == EINTR) {
                return 0;
            }
            sprintf(osm_error_str, "osm_task_server_recv: select failed: %s", strerror(errno));
            return ERR_SOCKET;
        }
        if(err > 0) {
            SLIST_FOREACH(session, &ts->sessions, entries) {
                if(FD_ISSET(session->sock, &readfds)) {
                    err = osm_task_server_handle(ts, session);
                    if(err != 0) {
                        SLIST_INSERT_HEAD(&ts->cleanup, session, entries);
                        return err;
                    }
                }
            }
        }
    }

    return 0;
}

int osm_task_server_loop(osm_task_server_t *ts, bool wait) {
    int err;

    // select and accept the server socket
    // if a new client socket is connected, allocate a session and
    // add it to ts->sessions
    err = osm_task_server_accept(ts);
    if(err != 0) {
        return err;
    }

    // for each session without a task assigned
    // get the next task
    // pb_encode it
    // send it to the socket
    err = osm_task_server_send_tasks(ts);
    if(err != 0) {
        return err;
    }

    // find sockets with waiting data
    // call recv
    // pb_decode the TaskStatus message
    err = osm_task_server_recv(ts, wait);
    if(err != 0) {
        return err;
    }

    // for each session in cleanup, remove it from sessions and run
    // destroy
    err = osm_task_server_cleanup(ts);
    if(err != 0) {
        return err;
    }

    return 0;
}

int osm_task_server_send_tasks(osm_task_server_t *ts) {
    osmnano_Task task_pb;
    osm_session_t *session;
    osm_task_t *task;
    pb_ostream_t ostream;
    uint8_t buf[65536];
    bool ok;
    int err;
    int offset;


    if(osm_task_server_empty(ts)) {
        return 0;
    }

    SLIST_FOREACH(session, &ts->sessions, entries) {
        if(session->task == NULL) {
            err = osm_task_server_get(ts, &task);
            if(err == ERR_NO_TASKS) {
                return 0;
            }
            if(err != 0) {
                return err;
            }
            session->task = task;

            task_pb.id = task->id;
            strcpy(task_pb.filename, task->fb.filename);
            task_pb.data_offset = task->fb.data_offset;
            task_pb.data_size = task->fb.data_size;
            task_pb.blob_header = task->fb.blob_header;

            ostream = pb_ostream_from_buffer(buf, 65536);
            ok = pb_encode(&ostream, osmnano_Task_fields, &task_pb);
            if(!ok) {
                sprintf(osm_error_str, "osm_task_server_send_next_task: encoding Task failed: %s", PB_GET_ERROR(&ostream));
                return ERR_PB_ENCODE;
            }

            offset = 0;
            while(offset < ostream.bytes_written) {
                err = send(session->sock, (buf + offset), (ostream.bytes_written - offset), 0);
                if(err == -1) {
                    sprintf(osm_error_str, "osm_task_server_send_next_task: send failed: %s", strerror(errno));
                    return ERR_SOCKET;
                }
                if(err == 0) {
                    break;
                }
                offset += err;
            }

            if(offset != ostream.bytes_written) {
                sprintf(osm_error_str, "osm_task_server_send_next_task: didn't send the right number of bytes: %d != %zu", offset, ostream.bytes_written);
                return ERR_SHORT_WRITE;
            }
        }
    }

    return 0;
}

int osm_task_server_handle(osm_task_server_t *ts, osm_session_t *session) {
    osmnano_TaskStatus task_status;

    pb_istream_t istream;

    uint8_t buf[65536];
    ssize_t len;
    bool ok;
    int err = 0;

    len = recv(session->sock, buf, 65536, 0);
    if(len == -1) {
        sprintf(osm_error_str, "osm_task_server_handle: recv failed: %s", strerror(errno));
        return ERR_SOCKET;
    }
    if(len == 0) {
        // client closed connection
        printf("client closed connection\n");
        SLIST_INSERT_HEAD(&ts->cleanup, session, entries);
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
            osm_fileblock_destroy(&session->task->fb);
            free(session->task);
            session->task = NULL;
            break;
        case osmnano_TaskStatus_Status_FAILED:
            printf("Task %lu failed\n", task_status.id);
            osm_fileblock_destroy(&session->task->fb);
            free(session->task);
            session->task = NULL;
            break;
        default:
            printf("Task %lu unknown status %d\n", task_status.id, task_status.status);
    }

    ts->completed_tasks++;

    return err;
}

bool osm_task_server_empty(osm_task_server_t *ts) {
    return STAILQ_EMPTY(&ts->queue);
}

int osm_task_server_inflight(osm_task_server_t *ts) {
    return (ts->submitted_tasks - ts->completed_tasks);
}

int osm_task_server_wait(osm_task_server_t *ts) {
    osm_session_t *session;
    int wstatus;
    pid_t err;
    int i;

    while(!SLIST_EMPTY(&ts->sessions)) {
        session = SLIST_FIRST(&ts->sessions);
        SLIST_REMOVE_HEAD(&ts->sessions, entries);
        close(session->sock);
        free(session);
    }

    for(i = 0; i < ts->num_workers; i++) {
        if(ts->pids[i] == 0) {
            continue;
        }
        err = waitpid(ts->pids[i], &wstatus, 0);
        if(err == -1) {
            sprintf(osm_error_str, "osm_task_server_wait: waitpid failed: %s", strerror(errno));
            return ERR_WAITPID;
        }
    }
    return 0;
}

void osm_task_server_destroy(osm_task_server_t *ts) {
    osm_task_t *task;

    while(osm_task_server_get(ts, &task) == 0) {
        osm_fileblock_destroy(&task->fb);
        free(task);
    }

    osm_task_server_cleanup(ts);

    close(ts->sock);
    freeaddrinfo(ts->addrs);
}

int osm_task_server_cleanup(osm_task_server_t *ts) {
    osm_session_t *session;

    while(!SLIST_EMPTY(&ts->cleanup)) {
        session = SLIST_FIRST(&ts->cleanup);
        SLIST_REMOVE_HEAD(&ts->cleanup, entries);
        SLIST_REMOVE(&ts->sessions, session, osm_session_s, entries);
        close(session->sock);
        free(session);
    }

    return 0;
}
