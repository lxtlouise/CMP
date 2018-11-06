#include "DataTypes.h"

Cpu cpu;

void init_tile(Tile *tile){
    tile->L1_cache = cache_create(1 << config.n1, 1 << config.b, 1 << config.a1);
    tile->L2_cache = cache_create(1 << config.n2, 1 << config.b, 1 << config.a2);
}

void init_cpu(){
    int i;
    cpu.n_tiles = 1 << config.p;
    cpu.tiles = (Tile*)malloc(sizeof(Tile) * cpu.n_tiles);
    for(i=0; i<cpu.n_tiles; i++){
        init_tile(&(cpu.tiles[i]));
    }
}
