#include "cmp.h"


int max_delay(int delay1, int delay2){
    return delay1>delay2 ? delay1 : delay2;
}
int tile2tile_delay(Tile *src, Tile *dst){
    if(src==NULL || dst==NULL)
        return 0;
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

void L2_make_block_shared(int* delay, Tile *tile, struct cache_blk_t *l2_block, int except_core_id){
    int i;
    for(i=0; i<cpu.n_tiles; i++){
        if(l2_block->bit_vec[i]>0 && i!=except_core_id){
            struct cache_blk_t *l1_block;
            if(cache_retrieve_block(cpu.tiles[i].L1_cache, &l1_block, l2_block->block_address)==CACHE_SAME_BLOCK && l1_block->valid){
                l1_block->dirty = 0;
            }
        }
    }
}

void notify_owner_blocks_from_local(int* delay, Tile *tile, struct cache_blk_t *l2_block, mem_access_t *access, int except_core_id){
    int i;
    int max_delay = 0;
    for(i=0; i<cpu.n_tiles; i++){
        if(l2_block->bit_vec[i]>0 && i!=except_core_id){
            struct cache_blk_t *l1_block;
            if(cache_retrieve_block(cpu.tiles[i].L1_cache, &l1_block, l2_block->block_address)==CACHE_SAME_BLOCK && l1_block->valid){
                char msg[64];
                int d = tile2tile_delay(tile, &(cpu.tiles[i]));
                cpu.tiles[access->core_id].short_messages += 1;
                sprintf(msg, "invalidate block in tile %i", i);
                log_generic(access, msg, *delay + d);
                d *= 2;
                cpu.tiles[access->core_id].short_messages += 1;
                sprintf(msg, "ack from tile %i", i);
                log_generic(access, msg, *delay + d);
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


void get_block_from_memory(int* delay, mem_access_t* access, Tile *tile, Tile *home_tile){
    *delay += tile2tile_delay(tile, home_tile); // message from tile to home tile
    *delay += tile2tile_delay(home_tile, &(cpu.tiles[0])); // message from home tile to memory controller
    *delay += config.d1; // get block from memory
    *delay += tile2tile_delay(&(cpu.tiles[0]), home_tile); // block from memory controller to home tile
    *delay += tile2tile_delay(home_tile, tile); // message from home tile to tile
    cpu.tiles[access->core_id].short_messages += 2;
    cpu.tiles[access->core_id].long_messages += 3;
}

void push_block_to_memory(int* delay, Tile *tile, struct cache_blk_t *l2_block, mem_access_t* access){
    //log_L2_write_back(access, 0);
    cpu.tiles[access->core_id].long_messages += 2;

    log_generic(access, "push block to memory", 0);
    // push block to  memory
}

void make_block_exclusive(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i, to_hit = 0;
    Tile *home_tile = get_home_tile(access->address);
    struct cache_blk_t *l2_block;
    //*delay += tile2tile_delay(tile, home_tile);
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, access->address);
    *delay += l2_block->block_delay;
    log_generic(access, "accessed L2 block", *delay);
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            to_hit = 1;
            home_tile->L2_cache->n_hits++;
            log_generic(access, "L2 hit", *delay);
            if(l2_block->block_state!=BLOCK_M){
                *delay += tile2tile_delay(tile, home_tile);
                cpu.tiles[access->core_id].short_messages += 1;
                log_generic(access, "returned sharers and data", *delay);
                notify_owner_blocks_from_local(delay, home_tile, l2_block, access, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
                cpu.tiles[access->core_id].short_messages += 1;
                log_generic(access, "revise entry", l2_block->block_delay);
            } else if(l2_block->bit_vec[tile->index]==0){
                *delay += tile2tile_delay(tile, home_tile);
                cpu.tiles[access->core_id].short_messages += 1;
                log_generic(access, "returned sharers and data", *delay);
                notify_owner_blocks_from_local(delay, home_tile, l2_block, access, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
                cpu.tiles[access->core_id].short_messages += 1;
                log_generic(access, "revise entry", l2_block->block_delay);
            }
        }
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_M;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 1);
    //l2_block->block_delay = *delay;
    //log_L2_hit(access, l2_block->block_delay);
    //log_generic(access, "L2 hit", l2_block->block_delay);
    /*if(to_hit){
        if(l2_block->block_delay==0)
            log_L2_hit(access);
        else
            log_L2_miss(access);
    }*/
    cpu.tiles[access->core_id].long_messages += 1;
    *delay += config.d;
}

void request_shared_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i, to_hit = 0;
    Tile *home_tile = get_home_tile(access->address);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, access->address);
    //*delay += tile2tile_delay(tile, home_tile); // get to home tile
    *delay += l2_block->block_delay;
    log_generic(access, "accessed L2 block", *delay);
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            to_hit = 1;
            home_tile->L2_cache->n_hits++;
            log_generic(access, "L2 hit", *delay);
            if(l2_block->block_state==BLOCK_S){
                l2_block->block_delay = *delay;
                *delay += tile2tile_delay(tile, home_tile);
                cpu.tiles[access->core_id].long_messages += 1;
                log_generic(access, "returned data", *delay);
            } else {
                if(l2_block->bit_vec[tile->index]==0){
                    *delay += tile2tile_delay(tile, home_tile);
                    cpu.tiles[access->core_id].short_messages += 1;
                    log_generic(access, "returned owner", *delay);
                    *delay += tile2tile_delay(find_single_owner(l2_block), tile);
                    cpu.tiles[access->core_id].short_messages += 1;
                    log_generic(access, "request to owner", *delay);
                    l2_block->block_delay = *delay + tile2tile_delay(tile, home_tile);
                    cpu.tiles[access->core_id].long_messages += 1;
                    log_generic(access, "revise entry", l2_block->block_delay);
                    *delay += tile2tile_delay(find_single_owner(l2_block), tile);
                    cpu.tiles[access->core_id].long_messages += 1;
                    log_generic(access, "return data", *delay);
                    L2_make_block_shared(delay, tile, l2_block, tile->index);
                }
            }
        } else {
            home_tile->L2_cache->n_misses++;
            log_generic(access, "L2 miss", *delay);
            //log_L2_miss(access, l2_block->block_delay);
            cache_block_init(home_tile->L2_cache, l2_block, access->address);
            get_block_from_memory(delay, access, home_tile, home_tile);
            log_generic(access, "got block from memory", *delay);
            l2_block->block_delay = *delay;
            *delay += tile2tile_delay(tile, home_tile);
            cpu.tiles[access->core_id].long_messages += 1;
            log_generic(access, "returned data", *delay);
        }
    } else {
        home_tile->L2_cache->n_misses++;
        log_generic(access, "L2 miss", *delay);
        //log_L2_miss(access, l2_block->block_delay);
        if(l2_block->valid){
            notify_owner_blocks_from_local(delay, tile, l2_block, access, -1);
            L2_invalidate_block(delay, home_tile, l2_block, -1);
            push_block_to_memory(delay, home_tile, l2_block, access);
        }
        cache_block_init(home_tile->L2_cache, l2_block, access->address);
        get_block_from_memory(delay, access, home_tile, home_tile);
        log_generic(access, "got block from memory", *delay);
        l2_block->block_delay = *delay;
        *delay += tile2tile_delay(tile, home_tile);
        cpu.tiles[access->core_id].long_messages += 1;
        log_generic(access, "returned data", *delay);
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, access->address, 0);
    //l2_block->block_delay = *delay;
    //log_generic(access, "L2 hit", l2_block->block_delay);
    //log_L2_hit(access, l2_block->block_delay);
    /*if(to_hit){
        if(l2_block->block_delay==0)
            log_L2_hit(access);
        else
            log_L2_miss(access);
    }*/
    //*delay += tile2tile_delay(tile, home_tile);
    cpu.tiles[access->core_id].long_messages += 1;
    *delay += config.d;
}

void request_exclusive_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i, to_hit = 0;
    Tile *home_tile = get_home_tile(access->address);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, access->address);
    //*delay += tile2tile_delay(tile, home_tile); // get to home tile
    *delay += l2_block->block_delay;
    log_generic(access, "accessed L2 block", *delay);
    if(r == CACHE_SAME_BLOCK){
        if(l2_block->valid){
            to_hit = 1;
            home_tile->L2_cache->n_hits++;
            log_generic(access, "L2 hit", *delay);
            if(l2_block->block_state!=BLOCK_M){
                *delay += tile2tile_delay(tile, home_tile);
                cpu.tiles[access->core_id].long_messages += 1;
                log_generic(access, "return sharers and data", *delay);
                notify_owner_blocks_from_local(delay, tile, l2_block, access, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
                cpu.tiles[access->core_id].short_messages += 1;
                log_generic(access, "revise entry", l2_block->block_delay);
            } else if(l2_block->bit_vec[tile->index]==0){
                *delay += tile2tile_delay(tile, home_tile);
                cpu.tiles[access->core_id].long_messages += 1;
                log_generic(access, "return sharers and data", *delay);
                notify_owner_blocks_from_local(delay, tile, l2_block, access, tile->index);
                L2_invalidate_block(delay, home_tile, l2_block, tile->index);
                l2_block->block_delay = *delay + max_delay(tile2tile_delay(tile, home_tile), config.d);
                cpu.tiles[access->core_id].short_messages += 1;
                log_generic(access, "revise entry", l2_block->block_delay);
            }
        } else {
            home_tile->L2_cache->n_misses++;
            log_generic(access, "L2 miss", *delay);
            cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
            get_block_from_memory(delay, access, home_tile, home_tile);
            log_generic(access, "got block from memory", *delay);
            *delay += tile2tile_delay(tile, home_tile);
            cpu.tiles[access->core_id].long_messages += 1;
            log_generic(access, "returned data", *delay);
            l2_block->block_delay = *delay + config.d;
        }
    } else {
        home_tile->L2_cache->n_misses++;
        log_generic(access, "L2 miss", *delay);
        if(l2_block->valid){
            notify_owner_blocks_from_local(delay, tile, l2_block, access, -1);
            L2_invalidate_block(delay, home_tile, l2_block, -1);
            push_block_to_memory(delay, home_tile, l2_block, access);
        }
        cache_block_init(home_tile->L2_cache, l2_block, block->block_address);
        get_block_from_memory(delay, access, home_tile, home_tile);
        log_generic(access, "got block from memory", *delay);
        *delay += tile2tile_delay(tile, home_tile);
        cpu.tiles[access->core_id].long_messages += 1;
        log_generic(access, "returned data", *delay);
        l2_block->block_delay = *delay + config.d;
    }
    l2_block->bit_vec[tile->index] = 1;
    l2_block->block_state = BLOCK_M;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 0);
    //l2_block->block_delay = *delay;
    //log_generic(access, "L2 hit", l2_block->block_delay);
    /*if(to_hit){
        if(l2_block->block_delay==0)
            log_L2_hit(access);
        else
            log_L2_miss(access);
    }*/
    //*delay += tile2tile_delay(tile, home_tile);
    cpu.tiles[access->core_id].long_messages += 1;
    *delay += config.d;
}

void evict_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i;
    Tile *home_tile = get_home_tile(block->block_address);
    int bdelay = 0;
    cpu.tiles[access->core_id].long_messages += 1;
    bdelay += tile2tile_delay(tile, home_tile);
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    if(r!=CACHE_SAME_BLOCK){
        //printf("!");
        return;
    }
    bdelay += l2_block->block_delay;
    log_generic(access, "L1 evict", l2_block->block_delay);
    //push_block_to_memory(&bdelay, home_tile, l2_block, access);
    l2_block->bit_vec[tile->index] = 0;
    if(l2_block->block_state == BLOCK_M)
        l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 1);
    l2_block->dirty = 1;
    l2_block->block_delay = bdelay;
}

void invalidate_block(int* delay, Tile *tile, struct cache_blk_t *block, mem_access_t* access){
    int i;
    Tile *home_tile = get_home_tile(block->block_address);
    int bdelay = 0;
    bdelay += tile2tile_delay(tile, home_tile);
    cpu.tiles[access->core_id].short_messages += 1;
    struct cache_blk_t *l2_block;
    int r = cache_retrieve_block(home_tile->L2_cache, &l2_block, block->block_address);
    if(r!=CACHE_SAME_BLOCK){
        //printf("!");
        return;
    }
    bdelay += l2_block->block_delay;
    log_generic(access, "L1 invalidate", l2_block->block_delay);
    l2_block->bit_vec[tile->index] = 0;
    if(l2_block->block_state == BLOCK_M)
        l2_block->block_state = BLOCK_S;
    cache_apply_access(home_tile->L2_cache, l2_block, block->block_address, 0);
    l2_block->dirty = 0;
    l2_block->block_delay = bdelay;
}

void request_L2_block(int* delay, Tile *tile, unsigned long address){
    Tile *home_tile = get_home_tile(address);
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
        access->L1_penalty = -1;
        log_generic(access, "STARTED", 0);
        access->status = STATUS_TO_L2;
        requests[i].delay = 0;
        struct cache_blk_t *block;
        int r = cache_retrieve_block(tile->L1_cache, &block, access->address);
        if(r==CACHE_SAME_BLOCK){
            if(block->valid){
                if(access->access_type==0){ // SIMPLE L1 READ HIT
                    tile->L1_cache->n_hits++;
                    log_generic(access, "L1 hit", 0);
                    access->status = STATUS_TO_COMPLETE;
                    L1_read_hit(&(requests[i].delay), tile, block, access);
                    log_generic(requests[i].access, "COMPLETED", requests[i].delay);
                } else {
                    if(block->dirty){ // SIMPLE L1 WRITE HIT
                        tile->L1_cache->n_hits++;
                        log_generic(access, "L1 hit", 0);
                        access->status = STATUS_TO_COMPLETE;
                        L1_write_hit(&(requests[i].delay), tile, block, access);
                        log_generic(requests[i].access, "COMPLETED", requests[i].delay);
                    } else { // L1 WRITE MISS BECAUSE NOT EXCLUSIVE
                        tile->L1_cache->n_misses++;
                        request_L2_block(&(requests[i].delay), tile, access->address);
                        cpu.tiles[access->core_id].short_messages += 1;
                        access->L1_penalty = cpu.clock;
                        log_generic(access, "L1 miss", requests[i].delay);
                    }
                }
            } else {
                if(access->access_type==0){ // L1 READ MISS
                    tile->L1_cache->n_misses++;
                    request_L2_block(&(requests[i].delay), tile, access->address);
                    cpu.tiles[access->core_id].short_messages += 1;
                    access->L1_penalty = cpu.clock;
                    log_generic(access, "L1 miss", 0);
                } else { // L1 WRITE MISS
                    tile->L1_cache->n_misses++;
                    request_L2_block(&(requests[i].delay), tile, access->address);
                    cpu.tiles[access->core_id].short_messages += 1;
                    access->L1_penalty = cpu.clock;
                    log_generic(access, "L1 miss", 0);
                }
            }
        } else {
            tile->L1_cache->n_misses++;
            request_L2_block(&(requests[i].delay), tile, access->address);
            cpu.tiles[access->core_id].short_messages += 1;
            access->L1_penalty = cpu.clock;
            log_generic(access, "L1 miss", 0);
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
    log_generic(access, "request to home node", 0);
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
    access->L1_penalty = cpu.clock + request->delay - access->L1_penalty;
    log_generic(request->access, "COMPLETED", request->delay);
}
