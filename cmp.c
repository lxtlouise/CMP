#include "cmp.h"

int tile2tile_delay(Tile *src, Tile *dst){
    return config.C * (abs(src->index_x - dst->index_x) + abs(src->index_y - dst->index_y));
}




void process_issued_requests(memory_request_t *requests, int n_requests){
    int i;
    // here, we are supposed to calculate the delay of every memory request
    for(i=0; i<n_requests; i++){
        requests[i].delay = 0;
    }
}
