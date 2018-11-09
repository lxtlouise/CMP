#include "cmp.h"

int tile2tile_delay(Tile *src, Tile *dst){
    return config.C * (abs(src->index_x - dst->index_x) + abs(src->index_y - dst->index_y));
}

Tile* get_home_tile(unsigned long address){
    address = address >> 12;
    address = address % (cpu.n_tiles);
    return &(cpu.tiles[address]);
}

int read_miss_L1(Tile *requesting_tile, Tile* home_tile, mem_access_t *access){

}


void process_issued_requests(memory_request_t *requests, int n_requests){
    int i;
    // here, we are supposed to calculate the delay of every memory request
    for(i=0; i<n_requests; i++){
        Tile *tile = requests[i].tile;
        mem_access_t *access = requests[i].access;
        access->delay = 0;
        struct cache_blk_t *block;
        int r = cache_retrieve_block(tile->L1_cache, &block, access->address);
        switch(r){
        case CACHE_HIT:
            if(access->access_type==0){ // L1 READ HIT
                access->delay = 0;
            } else {                    // L1 WRITE HIT
            }
            break;
        case CACHE_MISS_NO_EVICT:
            if(access->access_type==0){ // L1 READ MISS WITHOUT NEEDING TO EVICT BLOCK

            } else {                    // L1 WRITE MISS WITHOUT NEEDING TO EVICT BLOCK

            }
            break;
        case CACHE_MISS_EVICT:
            if(access->access_type==0){ // L1 READ MISS WITH NEEDING TO EVICT BLOCK

            } else {                    // L1 WRITE MISS WITH NEEDING TO EVICT BLOCK

            }
            break;
        default:
            break;
        }
    }
}
