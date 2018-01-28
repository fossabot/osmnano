#ifndef OSM_ERROR_H
#define OSM_ERROR_H

enum {
    OK = 0,
    ERR_READ_HEADER_SIZE,
    ERR_MALLOC,
    ERR_READ_HEADER_DATA,
    ERR_PB_DECODE_HEADER,
    ERR_READ_BLOB_DATA,
    ERR_EOF,
    ERR_UNCOMPRESS,
    ERR_SEEK,
    ERR_SOCKET,
    ERR_NO_TASKS,
    ERR_PB_DECODE,
    ERR_PB_ENCODE,
    ERR_SHORT_WRITE,
    ERR_WAITPID
} osm_error;

char osm_error_str[1024];
char *osm_get_error(void);

#endif
