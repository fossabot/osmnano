#ifndef TASK_SERVER_H
#define TASK_SERVER_H
#include "fileblock.h"

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TASK_SERVER_PORT "14004"


typedef struct osm_task_s osm_task_t;
struct osm_task_s {
    osm_fileblock_t fb;
    STAILQ_ENTRY(osm_task_s) entries;
};

typedef struct osm_session_s osm_session_t;
struct osm_session_s {
    int sock;
    int err;
    struct sockaddr addr;
    socklen_t addrlen;
    SLIST_ENTRY(osm_session_s) entries;
};

struct osm_task_server_s {
    STAILQ_HEAD(osm_task_q_head, osm_task_s) queue;
    SLIST_HEAD(osm_sessions_head, osm_session_s) sessions;
    SLIST_HEAD(osm_cleanup_head, osm_session_s) cleanup;
    struct addrinfo *addrs;
    int sock;
};
typedef struct osm_task_server_s osm_task_server_t;


int osm_task_server_init(osm_task_server_t *ts);

int osm_task_server_add(osm_task_server_t *ts, osm_task_t *task);
int osm_task_server_get(osm_task_server_t *ts, osm_task_t **task);

int osm_task_server_loop(osm_task_server_t *ts);
int osm_task_server_handle(osm_task_server_t *ts, osm_session_t *session);
bool osm_task_server_empty(osm_task_server_t *ts);
int osm_task_server_wait(osm_task_server_t *ts);
void osm_task_server_destroy(osm_task_server_t *ts);


#endif
