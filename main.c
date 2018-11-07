#include <stdio.h>
#include <stdlib.h>
#include "DataTypes.h"
int main()
{
    read_configfile("config.txt");
    init_cpu();
    read_trace_file("trace0");
    int clock = 0;
    int i,j;
    while(1){
        int have_more_requests = 0;
        for(i=0; i<cpu.n_tiles; i++){
            Tile *tile = &(cpu.tiles[i]);
            if(tile->access_index <= tile->n_accesses)
                have_more_requests++;
            mem_access_t *access = get_tile_next_access(tile, clock);
            if(access){
                // every request on the same clock
                print_mem_access(tile, access);
                // if there are delays they should be added to tile->delay_offset
            }
        }
        if(have_more_requests==0)
            break;
        clock++;
    }
    return 0;
}
