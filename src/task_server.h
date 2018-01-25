#ifndef TASK_SERVER_H
#define TASK_SERVER_H
#include "fileblock.h"

#include <sys/queue.h>

#define SOCKET_NAME "/tmp/osmnano.sock"


typedef struct osm_task_s osm_task_t;
struct osm_task_s {
    osm_fileblock_t fb;
    STAILQ_ENTRY(osm_task_s) entries;
};

struct osm_task_server_s {
    STAILQ_HEAD(osm_task_q_head, osm_task_s) queue;
    int sock;
};
typedef struct osm_task_server_s osm_task_server_t;


int osm_task_server_init(osm_task_server_t *ts);

int osm_task_server_add(osm_task_server_t *ts, osm_task_t *task);
int osm_task_server_get(osm_task_server_t *ts, osm_task_t **task);

int osm_task_server_loop(osm_task_server_t *ts);
int osm_task_server_wait(osm_task_server_t *ts);
void osm_task_server_destroy(osm_task_server_t *ts);


#endif
