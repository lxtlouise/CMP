#include "cmp.h"

int tile2tile_delay(Tile *src, Tile *dst){
    return config.C * (abs(src->index_x - dst->index_x) + abs(src->index_y - dst->index_y));
}

Tile* get_home_tile(unsigned long address){
    address = address >> 12;
    address = address % (cpu.n_tiles);
    return &(cpu.tiles[address]);
}

void L1_write_back_block(int* delay, Tile *tile, struct cache_blk_t *l1_block){
    Tile *home_tile = get_home_tile(block->block_address);
    *delay += tile2tile_delay(tile, home_tile); // message from tile to home tile
}

void L2_invalidate_block(int* delay, Tile *tile, struct cache_blk_t *l2_block, int ){
    int i;
    for(i=0; i<cpu.n_tiles; i++){
        if(l2_block->bit_vec[i]>0){
            struct cache_blk_t *l1_block;
            if(cache_retrieve_block(cpu.tiles[i].L1_cache, &l1_block, l2_block->block_address)==CACHE_SAME_BLOCK && l1_block->valid){
                if(l1_block->dirty)
                    L1_write_back_block(delay, &(cpu.tiles[i]), l1_block);
                l1_block->valid = 0;
            }
            l2_block->bit_vec[i] = 0;
        }
    }
}

void L1_read_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + 0;
    cache_apply_access(tile->L1_cache, block, access->address, access->access_type);
}

void L1_write_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + 0;
    cache_apply_access(tile->L1_cache, block, access->address, access->access_type);
}

void L2_read_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + config.d;
    cache_apply_access(tile->L2_cache, block, access->address, access->access_type);
}

void L2_write_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + config.d;
    cache_apply_access(tile->L2_cache, block, access->address, access->access_type);
}

void L2_read_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + 0;
    cache_apply_access(tile->L2_cache, block, block->block_address, 0);
}

void L2_write_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + 0;
    cache_apply_access(tile->L2_cache, block, block->block_address, 1);
}

void get_block_from_memory(int* delay, Tile *tile, Tile *home_tile){
    *delay += tile2tile_delay(tile, home_tile); // message from tile to home tile
    *delay += tile2tile_delay(home_tile, &(cpu.tiles[0])); // message from home tile to memory controller
    *delay += config.d1; // get block from memory
    *delay += tile2tile_delay(&(cpu.tiles[0]), home_tile); // block from memory controller to home tile
    *delay += tile2tile_delay(home_tile, tile); // message from home tile to tile
}

void make_block_exclusive(int* delay, Tile *tile, struct cache_blk_t *block){
    *delay = *delay + 0;
}

void request_shared_block(int* delay, Tile *tile, struct cache_blk_t *block){
    int i;
    Tile *home_tile = get_home_tile(block->block_address);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            if(l2_block->block_state!=BLOCK_S){
                if(l2_block->bit_vec[tile->index]==0){
                    L2_invalidate_block(delay, home_tile, l2_block);
                }
            }
        } else {
            cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
            get_block_from_memory(delay, tile, home_tile);
        }
    } else {
        if(l2_block->valid){
            L2_invalidate_block(delay, home_tile, l2_block);
        }
        cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
        get_block_from_memory(delay, tile, home_tile);
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 0);
    *delay += config.d;
}

void request_exclusive_block(int* delay, Tile *tile, struct cache_blk_t *block){

    int i;
    Tile *home_tile = get_home_tile(block->block_address);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            if(l2_block->block_state!=BLOCK_M){
                L2_invalidate_block(delay, home_tile, l2_block);
            }
        } else {
            cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
            get_block_from_memory(delay, tile, home_tile);
        }
    } else {
        if(l2_block->valid){
            L2_invalidate_block(delay, home_tile, l2_block);
        }
        cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
        get_block_from_memory(delay, tile, home_tile);
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_M;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 1);
    *delay += config.d;
}

void evict_block(int* delay, Tile *tile, struct cache_blk_t *block){
    *delay = *delay + 0;
}

void invalidate_block(int* delay, Tile *tile, struct cache_blk_t *block){
    *delay = *delay + 0;
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
        if(r==CACHE_SAME_BLOCK){
            if(block->valid){
                if(access->access_type==0){ // SIMPLE L1 READ HIT
                    L1_read_hit(&(access->delay), tile, block, access);
                } else {
                    if(block->dirty){ // SIMPLE L1 WRITE HIT
                        L1_write_hit(&(access->delay), tile, block, access);
                    } else { // L1 WRITE HIT BUT NOT EXCLUSIVE
                        make_block_exclusive(&(access->delay), tile, block);// make block exclusive
                        L1_write_hit(&(access->delay), tile, block, access);
                    }
                }
            } else {
                if(access->access_type==0){ // L1 READ MISS
                    request_shared_block(&(access->delay), tile, block);// request shared block
                    //access->delay += config.d;
                    L1_read_hit(&(access->delay), tile, block, access);
                } else { // L1 WRITE MISS
                    request_exclusive_block(&(access->delay), tile, block);// request exclusive block
                    //access->delay += config.d;
                    L1_write_hit(&(access->delay), tile, block, access);
                }
            }
        } else {
            if(block->valid){
                if(block->dirty){
                    evict_block(&(access->delay), tile, block);// evict block
                    if(access->access_type==0){ // L1 READ MISS
                        request_shared_block(&(access->delay), tile, block);// request shared block
                        //access->delay += config.d;
                        L1_read_hit(&(access->delay), tile, block, access);
                    } else { // L1 WRITE MISS
                        request_exclusive_block(&(access->delay), tile, block);// request exclusive block
                        //access->delay += config.d;
                        L1_write_hit(&(access->delay), tile, block, access);
                    }
                } else {
                    invalidate_block(&(access->delay), tile, block);// invalidate block and inform directory
                    if(access->access_type==0){ // L1 READ MISS
                        request_shared_block(&(access->delay), tile, block);// request shared block
                        //access->delay += config.d;
                        L1_read_hit(&(access->delay), tile, block, access);
                    } else { // L1 WRITE MISS
                        request_exclusive_block(&(access->delay), tile, block);// request exclusive block
                        //access->delay += config.d;
                        L1_write_hit(&(access->delay), tile, block, access);
                    }
                }
            } else {
                if(access->access_type==0){ // L1 READ MISS
                    request_shared_block(&(access->delay), tile, block);// request shared block
                    //access->delay += config.d;
                    L1_read_hit(&(access->delay), tile, block, access);
                } else { // L1 WRITE MISS
                    request_exclusive_block(&(access->delay), tile, block);// request exclusive block
                    //access->delay += config.d;
                    L1_write_hit(&(access->delay), tile, block, access);
                }
            }
        }
    }
}
