#ifndef TASK_SERVER_H
#define TASK_SERVER_H
#include "task.h"
#include "session.h"

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define TASK_SERVER_PORT "14004"
#define MAX_WORKERS 128


struct osm_task_server_s {
    STAILQ_HEAD(osm_task_q_head, osm_task_s) queue;
    SLIST_HEAD(osm_sessions_head, osm_session_s) sessions;
    SLIST_HEAD(osm_cleanup_head, osm_session_s) cleanup;
    struct addrinfo *addrs;
    int sock;
    pid_t pids[MAX_WORKERS];
    int num_workers;
    uint64_t submitted_tasks;
    uint64_t completed_tasks;
};
typedef struct osm_task_server_s osm_task_server_t;


int osm_task_server_init(osm_task_server_t *ts);

int osm_task_server_add(osm_task_server_t *ts, osm_task_t *task);
int osm_task_server_get(osm_task_server_t *ts, osm_task_t **task);

int osm_task_server_loop(osm_task_server_t *ts, bool wait);
int osm_task_server_accept(osm_task_server_t *ts);
int osm_task_server_recv(osm_task_server_t *ts, bool wait);

int osm_task_server_handle(osm_task_server_t *ts, osm_session_t *session);
int osm_task_server_send_tasks(osm_task_server_t *ts);
bool osm_task_server_empty(osm_task_server_t *ts);
int osm_task_server_inflight(osm_task_server_t *ts);
int osm_task_server_wait(osm_task_server_t *ts);
int osm_task_server_cleanup(osm_task_server_t *ts);
void osm_task_server_destroy(osm_task_server_t *ts);


#endif
