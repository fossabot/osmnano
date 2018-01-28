#ifndef TASK_WORKER_H
#define TASK_WORKER_H
#include "task_server.h"

#include <netdb.h>
#include <stdbool.h>

struct osm_task_worker_s {
    int sock;
};
typedef struct osm_task_worker_s osm_task_worker_t;


int osm_task_worker_fork(osm_task_server_t *ts);
int osm_task_worker_connect(osm_task_worker_t *worker, struct addrinfo *addr);
bool osm_task_worker_connected(osm_task_worker_t *worker);
int osm_task_worker_get_task(osm_task_worker_t *worker, osm_task_t *task);
int osm_task_worker_run(osm_task_worker_t *worker, osm_task_t *task);
int osm_task_worker_finish(osm_task_worker_t *worker, osm_task_t *task);
void osm_task_worker_destroy(osm_task_worker_t *worker);

#endif
