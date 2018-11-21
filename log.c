#include "log.h"
log_entry_t *log_head;
log_entry_t *log_tail;
void log_get_info(mem_access_t *access, Tile **tile, Tile **home_tile, struct cache_blk_t **L1_block, struct cache_blk_t **L2_block){
    *tile = &(cpu.tiles[access->core_id]);
    *home_tile = get_home_tile(access->address);
    cache_retrieve_block((*tile)->L1_cache, L1_block, access->address);
    cache_retrieve_block((*home_tile)->L2_cache, L2_block, access->address);
}
void init_log_entry(log_entry_t **entry){
    *entry = (log_entry_t*)malloc(sizeof(log_entry_t));
    (*entry)->clock = cpu.clock;
    (*entry)->next = NULL;
}
void log_operation(char* message, int delay){
    int clock = cpu.clock + delay;
    log_entry_t *log_current, *log_prev, *log_this;

    init_log_entry(&(log_this));
    log_this->clock = cpu.clock + delay;
    strncpy(log_this->msg, message, 256);

    if(log_head==NULL){
        log_head = log_this;
    } else {
        log_current = log_head;
        log_prev = NULL;
        while(log_current != NULL && log_current->clock<= clock){
            log_prev = log_current;
            log_current = log_current->next;
        }
        if(log_prev==NULL){
            log_this->next = log_head;
            log_head = log_this;
        } else {
            log_this->next = log_current;
            log_prev->next = log_this;
        }
    }
}

void print_recent_log(){
    while(log_head!=NULL && log_head->clock==cpu.clock){
        printf("%s\n", log_head->msg);
        if(log_head==log_tail){
            free(log_head);
            log_head = log_tail = NULL;
        } else {
            log_entry_t *l = log_head;
            log_head = log_head->next;
            free(l);
        }
    }
}

void log_generic(mem_access_t *access, char* message, int delay){
    char msg[256]; Tile *tile, *home_tile; struct cache_blk_t **L1_block, **L2_block;
    if(!debug_mode)
        return;
    log_get_info(access, &tile, &home_tile, &L1_block, &L2_block);
    sprintf(msg, "%-3i: %-6i %s 0x%08x  %s"
        , access->core_id, access->cycle, access->access_type==READ_ACCESS?"R":"W", access->address
        , message);
    log_operation(msg, delay);
}


