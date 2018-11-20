#include <stdio.h>
#include <stdlib.h>
#include "cmp.h"

void decrease_block_delay(struct cache_blk_t *block){
    if(block->block_delay>0)
        block->block_delay--;
}

int main()
{
    read_configfile("config.txt");
    init_cpu();
    read_trace_file("testtrace");
    int clock = 1;
    int i,j;
    memory_request_t *issued_requests = (memory_request_t*)malloc(sizeof(memory_request_t) * cpu.n_tiles);
    memory_request_t *enroute_requests = (memory_request_t*)malloc(sizeof(memory_request_t) * cpu.n_tiles * 2);
    memory_request_t *completed_requests = (memory_request_t*)malloc(sizeof(memory_request_t) * cpu.n_tiles * 2);
    int n_enroute_requests = 0;
    while(1){
        int n_issued_requests=0, n_completed_requests=0;
        printf("---Clock %i---\n", clock);
        /*for(i=0; i<n_enroute_requests; i++){
            Tile *tile = &(cpu.tiles[enroute_requests[i].access->core_id]);
            if(tile->delay_offset==0){
                issued_requests[n_issued_requests].access = enroute_requests[i].access;
                issued_requests[n_issued_requests].delay = 0;
                n_issued_requests++;
            }
            struct cache_blk_t *block;
            if(cache_retrieve_block(cpu.tiles[enroute_requests[i].access->core_id].L2_cache, block, enroute_requests[i].access->address)==CACHE_SAME_BLOCK){
                if(block->block_delay==0){
                    issued_requests[n_issued_requests].access = enroute_requests[i].access;
                    issued_requests[n_issued_requests].delay = 0;
                    n_issued_requests++;
                }
            }
        }*/
        int have_more_requests = 0;
        mem_access_t *enroute, *completed;
        for(i=0; i<cpu.n_tiles; i++){
            Tile *tile = &(cpu.tiles[i]);
            if(tile->is_finished==0 || tile->delay_offset>0)
                have_more_requests++;
            mem_access_t *access = get_tile_next_access(tile, &completed);
            if(completed!=NULL){
                if(completed->status==STATUS_TO_L2){
                    issued_requests[n_issued_requests].access = completed;
                    issued_requests[n_issued_requests].delay = 0;
                    n_issued_requests++;

                } else {
                    completed_requests[n_completed_requests].access = completed;
                    completed_requests[n_completed_requests].delay = 0;
                    print_mem_complete(&(completed_requests[n_completed_requests]));
                    n_completed_requests++;
                }
            }
            if(access){
                // access issued
                tile->delay_offset = 0;
                issued_requests[n_issued_requests].access = access;
                issued_requests[n_issued_requests].delay = 0;
                n_issued_requests++;

            }
        }
        process_issued_requests(issued_requests, n_issued_requests); // fill delays here
        for(i=0; i<n_issued_requests; i++){
            //print_mem_issue(&(issued_requests[i]));
            cpu.tiles[issued_requests[i].access->core_id].delay_offset = issued_requests[i].delay;
            if(issued_requests[i].delay==0 && issued_requests[i].access->status==STATUS_TO_COMPLETE){
                // current requests completed
                completed_requests[n_completed_requests].access = issued_requests[i].access;
                completed_requests[n_completed_requests].delay = 0;
                print_mem_complete(&(completed_requests[n_completed_requests]));
                n_completed_requests++;
            }
        }
        if(have_more_requests==0)
            break;
        clock++;
        for(i=0; i<cpu.n_tiles; i++){
            cache_traverse_blocks(cpu.tiles[i].L2_cache, decrease_block_delay);
        }
        printf("\n");

    }
    free(issued_requests);
    free(enroute_requests);
    free(completed_requests);
    return 0;
}


