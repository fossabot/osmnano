#include "fileformat.h"
#include "osmformat.h"
#include "ptrlist.h"

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>


int main(int argc, char **argv) {
    OSMPBF_BlobHeader header = OSMPBF_BlobHeader_init_default;
    OSMPBF_Blob blob = OSMPBF_Blob_init_default;
    decode_state_t state = {0};
    uint32_t header_size;
    int ret = 0;
    int fd;
    int err;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    /*
    if(ptrlist_init(&state.stringtable, LIST_INIT_SIZE) != 0) {
        fprintf(stderr, "Unable to initialize string table\n");
        return 1;
    }
    */

    fd = open(argv[1], O_RDONLY);

    //header.type.funcs.decode = &osm_header_type;
    //header.type.arg = &state;

    blob.raw.funcs.decode = &osm_blob_raw;
    blob.raw.arg = &state;

    blob.zlib_data.funcs.decode = &osm_blob_zlib;
    blob.zlib_data.arg = &state;

    blob.lzma_data.funcs.decode = &osm_blob_lzma;
    blob.OBSOLETE_bzip2_data.funcs.decode = &osm_blob_bzip2;

    while(1) {
        err = osm_decode_size(fd, &header_size);
        if(err != 0) {
            break;
        }

        err = osm_decode_header(fd, &header, header_size, &state);
        if(err != 0) {
            break;
        }

        err = osm_decode_blob(fd, &blob, header.datasize, &state);
        if(err != 0) {
            break;
        }
    }

    close(fd);

    ptrlist_destroy(&state.stringtable);

    return ret;
}
