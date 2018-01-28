#ifndef SESSION_H
#define SESSION_H
#include "task.h"
#include <sys/queue.h>

typedef struct osm_session_s osm_session_t;
struct osm_session_s {
    int sock;
    osm_task_t *task;
    SLIST_ENTRY(osm_session_s) entries;
};

int osm_session_init(osm_session_t *session);
void osm_session_destroy(osm_session_t *session);

#endif
