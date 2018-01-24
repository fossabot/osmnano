#include "task_server.h"
#include "fileblock.h"

#include <sys/queue.h>
#include <stdlib.h>


int osm_task_server_init(osm_task_server_t *ts) {
    STAILQ_INIT(&ts->queue);
    return 0;
}

int osm_task_server_add(osm_task_server_t *ts, osm_task_t *task) {
    STAILQ_INSERT_TAIL(&ts->queue, task, entries);
    return 0;
}

int osm_task_server_wait(osm_task_server_t *ts) {
    osm_task_t *task;

    while(!STAILQ_EMPTY(&ts->queue)) {
        task = STAILQ_FIRST(&ts->queue);
        STAILQ_REMOVE_HEAD(&ts->queue, entries);

        osm_fileblock_destroy(&task->fb);
        free(task);
    }
    return 0;
}

void osm_task_server_destroy(osm_task_server_t *ts) {
}
