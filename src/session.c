#include "session.h"

#include <unistd.h>

int osm_session_init(osm_session_t *session) {
    session->sock = 0;
    session->task = NULL;
    return 0;
}

void osm_session_destroy(osm_session_t *session) {
    if(session->sock > 0) {
        close(session->sock);
    }
}
