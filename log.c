#include "log.h"

void log_get_info(mem_access_t *access, Tile **tile, Tile **home_tile, struct cache_blk_t **L1_block, struct cache_blk_t **L2_block){
    *tile = &(cpu.tiles[access->core_id]);
    *home_tile = get_home_tile(access->address);
    cache_retrieve_block((*tile)->L1_cache, L1_block, access->address);
    cache_retrieve_block((*home_tile)->L2_cache, L2_block, access->address);
}
void log_L1_hit(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L1 HIT\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L1_miss(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L1 MISS\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L2_hit(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L2 HIT\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L2_miss(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L2 MISS\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L1_write_back(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L2 WRITE BACK\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L2_write_back(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L2 WRITE BACK\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L1_evict(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L1 EVICT\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}
void log_L1_invalidate(mem_access_t *access){
    Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    printf("%-3i: %-6i %s 0x%08x  L1 INVALIDATE\n"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address);
}

