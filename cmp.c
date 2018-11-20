#include "cmp.h"


int max_delay(int delay1, int delay2){
    return delay1>delay2 ? delay1 : delay2;
}
int tile2tile_delay(Tile *src, Tile *dst){
    return config.C * (abs(src->index_x - dst->index_x) + abs(src->index_y - dst->index_y));
}

Tile* get_home_tile(unsigned long address){
    address = address >> 12;
    address = address % (cpu.n_tiles);
    return &(cpu.tiles[address]);
}

void L1_write_back_block(int* delay, Tile *tile, struct cache_blk_t *l1_block){
    Tile *home_tile = get_home_tile(l1_block->block_address);
    *delay += tile2tile_delay(tile, home_tile); // message from tile to home tile
}

Tile* find_single_owner(struct cache_blk_t *l2_block){
    int i;
    int max_delay = 0;
    for(i=0; i<cpu.n_tiles; i++){
        if(l2_block->bit_vec[i]>0){
            return &(cpu.tiles[i]);
        }
    }
    return NULL;
}

void L2_invalidate_block(int* delay, Tile *tile, struct cache_blk_t *l2_block, int except_core_id){
    int i;
    //int max_delay = 0;
    for(i=0; i<cpu.n_tiles; i++){
        if(l2_block->bit_vec[i]>0 && i!=except_core_id){
            struct cache_blk_t *l1_block;
            if(cache_retrieve_block(cpu.tiles[i].L1_cache, &l1_block, l2_block->block_address)==CACHE_SAME_BLOCK && l1_block->valid){
                //if(l1_block->dirty)
                //    L1_write_back_block(delay, &(cpu.tiles[i]), l1_block);
                l1_block->valid = 0;
                //int d = 2*tile2tile_delay(tile, &(cpu.tiles[i]));
                //if(d>max_delay)
                //    max_delay = d;
            }
            l2_block->bit_vec[i] = 0;
        }
    }
    //*delay += max_delay;
}

void notify_owner_blocks_from_local(int* delay, Tile *tile, struct cache_blk_t *l2_block, int except_core_id){
    int i;
    int max_delay = 0;
    for(i=0; i<cpu.n_tiles; i++){
        if(l2_block->bit_vec[i]>0 && i!=except_core_id){
            struct cache_blk_t *l1_block;
            if(cache_retrieve_block(cpu.tiles[i].L1_cache, &l1_block, l2_block->block_address)==CACHE_SAME_BLOCK && l1_block->valid){
                int d = 2*tile2tile_delay(tile, &(cpu.tiles[i]));
                if(d>max_delay)
                    max_delay = d;
            }
        }
    }
    *delay += max_delay;
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

/*void L2_read_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + 0;
    cache_apply_access(tile->L2_cache, block, block->block_address, 0);
}

void L2_write_hit(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t *access){
    *delay = *delay + 0;
    cache_apply_access(tile->L2_cache, block, block->block_address, 1);
}*/

void get_block_from_memory(int* delay, Tile *tile, Tile *home_tile){
    *delay += tile2tile_delay(tile, home_tile); // message from tile to home tile
    *delay += tile2tile_delay(home_tile, &(cpu.tiles[0])); // message from home tile to memory controller
    *delay += config.d1; // get block from memory
    *delay += tile2tile_delay(&(cpu.tiles[0]), home_tile); // block from memory controller to home tile
    *delay += tile2tile_delay(home_tile, tile); // message from home tile to tile
}

void push_block_to_memory(int* delay, Tile *tile, struct cache_blk_t *l2_block, mem_access_t* access){
    log_L2_write_back(access);
    // push block to  memory
}

void make_block_exclusive(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i, to_hit = 0;
    Tile *home_tile = get_home_tile(block->block_address);
    struct cache_blk_t *l2_block;
    //*delay += tile2tile_delay(tile, home_tile);
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    *delay += l2_block->block_delay;
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            to_hit = 1;
            if(l2_block->block_state!=BLOCK_M){
                *delay += tile2tile_delay(tile, home_tile);
                notify_owner_blocks_from_local(delay, home_tile, l2_block, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
            } else if(l2_block->bit_vec[tile->index]==0){
                *delay += tile2tile_delay(tile, home_tile);
                notify_owner_blocks_from_local(delay, home_tile, l2_block, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
            }
        }
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_M;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 1);
    //l2_block->block_delay = *delay;
    if(to_hit){
        if(l2_block->block_delay==0)
            log_L2_hit(access);
        else
            log_L2_miss(access);
    }
    *delay += config.d;
}

void request_shared_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i, to_hit = 0;
    Tile *home_tile = get_home_tile(block->block_address);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    //*delay += tile2tile_delay(tile, home_tile); // get to home tile
    *delay += l2_block->block_delay;
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            to_hit = 1;
            if(l2_block->block_state==BLOCK_S){
                l2_block->block_delay = *delay;
                *delay += tile2tile_delay(tile, home_tile);
            } else {
                if(l2_block->bit_vec[tile->index]==0){
                    *delay += tile2tile_delay(tile, home_tile);
                    *delay += tile2tile_delay(find_single_owner(l2_block), tile);
                    l2_block->block_delay = *delay + tile2tile_delay(tile, home_tile);
                    *delay += tile2tile_delay(find_single_owner(l2_block), tile);
                    L2_invalidate_block(delay, tile, l2_block, tile->index);
                }
            }
        } else {
            log_L2_miss(access);
            cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
            get_block_from_memory(delay, home_tile, home_tile);
            l2_block->block_delay = *delay;
            *delay += tile2tile_delay(tile, home_tile);
        }
    } else {
        log_L2_miss(access);
        if(l2_block->valid){
            notify_owner_blocks_from_local(delay, home_tile, l2_block, -1);
            L2_invalidate_block(delay, home_tile, l2_block, -1);
            push_block_to_memory(delay, home_tile, l2_block, access);
        }
        cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
        get_block_from_memory(delay, home_tile, home_tile);
        l2_block->block_delay = *delay;
        *delay += tile2tile_delay(tile, home_tile);
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 0);
    //l2_block->block_delay = *delay;
    if(to_hit){
        if(l2_block->block_delay==0)
            log_L2_hit(access);
        else
            log_L2_miss(access);
    }
    //*delay += tile2tile_delay(tile, home_tile);
    *delay += config.d;
}

void request_exclusive_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i, to_hit = 0;
    Tile *home_tile = get_home_tile(block->block_address);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    //*delay += tile2tile_delay(tile, home_tile); // get to home tile
    *delay += l2_block->block_delay;
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            to_hit = 1;

            if(l2_block->block_state!=BLOCK_M){
                *delay += tile2tile_delay(tile, home_tile);
                notify_owner_blocks_from_local(delay, home_tile, l2_block, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
            } else if(l2_block->bit_vec[tile->index]==0){
                *delay += tile2tile_delay(tile, home_tile);
                notify_owner_blocks_from_local(delay, home_tile, l2_block, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
            }
        } else {
            log_L2_miss(access);
            cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
            get_block_from_memory(delay, home_tile, home_tile);
            *delay += tile2tile_delay(tile, home_tile);
            l2_block->block_delay = *delay + config.d;
        }
    } else {
        log_L2_miss(access);
        if(l2_block->valid){
            notify_owner_blocks_from_local(delay, home_tile, l2_block, -1);
            L2_invalidate_block(delay, home_tile, l2_block, -1);
            push_block_to_memory(delay, home_tile, l2_block, access);
        }
        cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
        get_block_from_memory(delay, home_tile, home_tile);
        *delay += tile2tile_delay(tile, home_tile);
        l2_block->block_delay = *delay + config.d;
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_M;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 0);
    //l2_block->block_delay = *delay;
    if(to_hit){
        if(l2_block->block_delay==0)
            log_L2_hit(access);
        else
            log_L2_miss(access);
    }
    //*delay += tile2tile_delay(tile, home_tile);
    *delay += config.d;
}

void evict_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i;
    log_L1_evict(access);
    Tile *home_tile = get_home_tile(block->block_address);
    int bdelay = 0;
    bdelay += tile2tile_delay(tile, home_tile);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    bdelay += l2_block->block_delay;
    //push_block_to_memory(&bdelay, home_tile, l2_block, access);
    l2_block->bit_vec[tile->index] = 0;
    //l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 1);
    l2_block->dirty = 1;
    l2_block->block_delay = bdelay;
}

void invalidate_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i;
    log_L1_invalidate(access);
    Tile *home_tile = get_home_tile(block->block_address);
    int bdelay = 0;
    bdelay += tile2tile_delay(tile, home_tile);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    bdelay += l2_block->block_delay;
    l2_block->bit_vec[tile->index] = 0;
    //l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 0);
    l2_block->dirty = 0;
    l2_block->block_delay = bdelay;
}

void request_L2_block(int* delay, Tile *tile, struct cache_blk_t *block){
    Tile *home_tile = get_home_tile(block->block_address);
    *delay = *delay + tile2tile_delay(tile, home_tile);
}


void process_issued_requests(memory_request_t *requests, int n_requests){
    int i;
    // here, we are supposed to calculate the delay of every memory request
    for(i=0; i<n_requests; i++){
        if(requests[i].access->status==STATUS_TO_L2){
            process_enroute_request(&(requests[i]));
            continue;
        } else if(requests[i].access->status==STATUS_TO_COMPLETE)
            continue;
        Tile *tile = &(cpu.tiles[requests[i].access->core_id]);
        mem_access_t *access = requests[i].access;
        access->status = STATUS_TO_L2;
        requests[i].delay = 0;
        struct cache_blk_t *block;
        int r = cache_retrieve_block(tile->L1_cache, &block, access->address);
        if(r==CACHE_SAME_BLOCK){
            if(block->valid){
                if(access->access_type==0){ // SIMPLE L1 READ HIT
                    tile->L1_cache->n_hits++;
                    log_L1_hit(access);
                    access->status = STATUS_TO_COMPLETE;
                    L1_read_hit(&(requests[i].delay), tile, block, access);
                } else {
                    if(block->dirty){ // SIMPLE L1 WRITE HIT
                        tile->L1_cache->n_hits++;
                        log_L1_hit(access);
                        access->status = STATUS_TO_COMPLETE;
                        L1_write_hit(&(requests[i].delay), tile, block, access);
                    } else { // L1 WRITE MISS BECAUSE NOT EXCLUSIVE
                        tile->L1_cache->n_misses++;
                        log_L1_miss(access);
                        request_L2_block(&(requests[i].delay), tile, block);
                    }
                }
            } else {
                if(access->access_type==0){ // L1 READ MISS
                    tile->L1_cache->n_misses++;
                    log_L1_miss(access);
                    request_L2_block(&(requests[i].delay), tile, block);
                } else { // L1 WRITE MISS
                    tile->L1_cache->n_misses++;
                    log_L1_miss(access);
                    request_L2_block(&(requests[i].delay), tile, block);
                }
            }
        } else {
            tile->L1_cache->n_misses++;
            log_L1_miss(access);
            request_L2_block(&(requests[i].delay), tile, block);
        }
        if(access->status==STATUS_TO_L2 && requests[i].delay==0)
            process_enroute_request(&(requests[i]));
    }
}

void process_enroute_request(memory_request_t *request){
    int i;
    // here, we are supposed to calculate the delay of every memory request
    Tile *tile = &(cpu.tiles[request->access->core_id]);
    mem_access_t *access = request->access;
    access->status = STATUS_TO_COMPLETE;
    request->delay = 0;
    struct cache_blk_t *block;
    int r = cache_retrieve_block(tile->L1_cache, &block, access->address);
    if(r==CACHE_SAME_BLOCK){
        if(block->valid){
            if(access->access_type==0){ // SIMPLE L1 READ HIT
            } else {
                if(block->dirty){ // SIMPLE L1 WRITE HIT
                } else { // L1 WRITE MISS BECAUSE NOT EXCLUSIVE
                    make_block_exclusive(&(request->delay), tile, block, access);// make block exclusive
                    L1_write_hit(&(request->delay), tile, block, access);
                }
            }
        } else {
            if(access->access_type==0){ // L1 READ MISS
                request_shared_block(&(request->delay), tile, block, access);// request shared block
                L1_read_hit(&(request->delay), tile, block, access);
            } else { // L1 WRITE MISS
                request_exclusive_block(&(request->delay), tile, block, access);// request exclusive block
                L1_write_hit(&(request->delay), tile, block, access);
            }
        }
    } else {
        if(block->valid){
            if(block->dirty){
                evict_block(&(request->delay), tile, block, access);// evict block
                if(access->access_type==0){ // L1 READ MISS
                    request_shared_block(&(request->delay), tile, block, access);// request shared block
                    L1_read_hit(&(request->delay), tile, block, access);
                } else { // L1 WRITE MISS
                    request_exclusive_block(&(request->delay), tile, block, access);// request exclusive block
                    L1_write_hit(&(request->delay), tile, block, access);
                }
            } else {
                invalidate_block(&(request->delay), tile, block, access);// invalidate block and inform directory
                if(access->access_type==0){ // L1 READ MISS
                    request_shared_block(&(request->delay), tile, block, access);// request shared block
                    L1_read_hit(&(request->delay), tile, block, access);
                } else { // L1 WRITE MISS
                    request_exclusive_block(&(request->delay), tile, block, access);// request exclusive block
                    L1_write_hit(&(request->delay), tile, block, access);
                }
            }
        } else {
            if(access->access_type==0){ // L1 READ MISS
                request_shared_block(&(request->delay), tile, block, access);// request shared block
                L1_read_hit(&(request->delay), tile, block, access);
            } else { // L1 WRITE MISS
                request_exclusive_block(&(request->delay), tile, block, access);// request exclusive block
                L1_write_hit(&(request->delay), tile, block, access);
            }
        }
    }
}
