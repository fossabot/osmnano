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
    ERR_UNCOMPRESS
} osm_error;

char osm_error_str[1024];
char *osm_get_error(void);

#endif
