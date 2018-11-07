#include <stdio.h>
#include <stdlib.h>
#include "cmp.h"
int main()
{
    read_configfile("config.txt");
    init_cpu();
    read_trace_file("trace0");
    int clock = 1;
    int i,j;
    memory_request_t *issued_requests = (memory_request_t*)malloc(sizeof(memory_request_t) * cpu.n_tiles);
    memory_request_t *completed_requests = (memory_request_t*)malloc(sizeof(memory_request_t) * cpu.n_tiles * 2);
    while(1){
        printf("---Clock %i---\n", clock);
        int n_issued_requests=0, n_completed_requests=0;
        int have_more_requests = 0;
        mem_access_t *completed;
        for(i=0; i<cpu.n_tiles; i++){
            Tile *tile = &(cpu.tiles[i]);
            if(tile->is_finished==0 || tile->delay_offset>0)
                have_more_requests++;
            mem_access_t *access = get_tile_next_access(tile, &completed);
            if(completed!=NULL){
                //previous request completed
                completed_requests[n_completed_requests].access = completed;
                completed_requests[n_completed_requests].tile = tile;
                completed_requests[n_completed_requests].delay = 0;
                print_mem_complete(&(completed_requests[n_completed_requests]));
                n_completed_requests++;
            }
            if(access){
                // access issued
                tile->delay_offset = 0;
                issued_requests[n_issued_requests].access = access;
                issued_requests[n_issued_requests].tile = tile;
                issued_requests[n_issued_requests].delay = 0;
                n_issued_requests++;

            }
        }
        process_issued_requests(issued_requests, n_issued_requests); // fill delays here
        for(i=0; i<n_issued_requests; i++){
            print_mem_issue(&(issued_requests[i]));
            issued_requests[i].tile->delay_offset = issued_requests[i].delay;
            if(issued_requests[i].delay==0){
                // current requests completed
                completed_requests[n_completed_requests].access = issued_requests[i].access;
                completed_requests[n_completed_requests].tile = issued_requests[i].tile;
                completed_requests[n_completed_requests].delay = 0;
                print_mem_complete(&(completed_requests[n_completed_requests]));
                n_completed_requests++;
            }
        }
        if(have_more_requests==0)
            break;
        clock++;
        printf("\n");
    }
    free(issued_requests);
    free(completed_requests);
    return 0;
}


