#ifndef _DATA_TYPES_H
#define _DATA_TYPES_H
#define MAX_LINE 4096
#define WHITE_SPACE " \t\n"
#define LINE_TERMINATOR "\n"
#define CONFIG_EQUAL "="
#define STATUS_TO_START 0
#define STATUS_TO_L2 1
#define STATUS_TO_COMPLETE 2

#define ACCESS_MAKE_EXCLUSIVE
#define ACCESS_REQUEST_EXCLUSIVE
#define ACCESS_REQUEST_SHARED

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

typedef struct _mem_access{
    int cycle;
    int core_id;
    int access_type;
    unsigned long address;
    int request_delay;
    int status;
}mem_access_t;

typedef struct _tile {
    int index;
    int index_x, index_y;
    cache_t* L1_cache;
    cache_t* L2_cache;

    int access_index;
    int delay_offset;
    mem_access_t *accesses;
    mem_access_t *enroute_access;
    int n_accesses;
    size_t accesses_capacity;
    int is_finished;
    int cycles_to_finish;
}Tile;

typedef struct _memory_request{
    //Tile *tile;
    mem_access_t *access;
    int delay;
}memory_request_t;

typedef struct _cpu{
    int width, height;
    Tile* tiles;
    int n_tiles;
    int clock;
}Cpu;

extern Config config;

extern Cpu cpu;


void read_configfile(char *fileName);

void init_tile(Tile *tile);

void init_cpu();

void read_trace_file(char *filename);

mem_access_t * get_tile_next_access(Tile *tile, mem_access_t **completed);

void print_mem_issue(memory_request_t *request);
void print_mem_complete(memory_request_t *request);

#include "log.h"
#endif // _DATA_TYPES_H
