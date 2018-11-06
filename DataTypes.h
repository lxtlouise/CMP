#ifndef _DATA_TYPES_H
#define _DATA_TYPES_H
#define MAX_LINE 4096
#define WHITE_SPACE " \t\n"
#define LINE_TERMINATOR "\n"
#define CONFIG_EQUAL "="
#include "cache.h"
typedef struct _config {
    int p; // 2^p tiles
    int n1; // 2^n1 bytes for L1
    int n2; // 2^n2 bytes for L2
    int b; // 2^b block size for both L1 and L2
    int a1; // 2^a1 associativity of L1
    int a2; // 2^a2 associativity of L2
    int C; // H * C delay for message between two tiles
    int d; // d delays for L2 hit
    int d1; // memory controller in tile0 has delay d1 to communicate with main memory
} Config;

extern Config config;

typedef struct _tile {
    int index_x, index_y;
    cache_t* L1_cache;
    cache_t* L2_cache;
}Tile;

typedef struct _cpu{
    Tile* tiles;
    int n_tiles;
}Cpu;

extern Cpu cpu;

void init_tile(Tile *tile);

void init_cpu();

void read_configfile(char *fileName);

#endif // _DATA_TYPES_H
