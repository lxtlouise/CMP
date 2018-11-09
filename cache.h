/* This file contains a functional simulator of an associative cache with LRU replacement*/
#ifndef _CACHE_H
#define _CACHE_H

#define READ_ACCESS 0
#define WRITE_ACCESS 1

#define CACHE_HIT 0
#define CACHE_MISS_NO_EVICT 1
#define CACHE_MISS_EVICT 2

#include <stdlib.h>
#include <stdio.h>

typedef struct _directory_entry_t{
    int *owner_tiles;
    int block_state;
}directory_entry_t;

struct cache_blk_t { // note that no actual data will be stored in the cache
  unsigned long tag;
  char valid;
  char dirty;
  unsigned LRU;	//to be used to build the LRU stack for the blocks in a cache set

  unsigned long block_address;
  directory_entry_t directory_entry;
};

typedef struct _cache_t {
	// The cache is represented by a 2-D array of blocks.
	// The first dimension of the 2D array is "nsets" which is the number of sets (entries)
	// The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;					// number of sets
  int blocksize;				// block size
  int assoc;					// associativity
  int miss_penalty;				// the miss penalty
  struct cache_blk_t **blocks;	// a pointer to the array of cache blocks
}cache_t;

//------------------------------


struct cache_access_pattern_t{
    cache_t* cache;
    unsigned long address;
    int access_type;
    int delay;
    struct cache_access_pattern_t* next;
};
cache_t * cache_create(int size, int blocksize, int assoc);

void updateLRU(cache_t *cp ,int index, int way);
unsigned long cache_getAddress(cache_t *cp, unsigned long set_index, unsigned long tag);
unsigned long cache_get_block_address(cache_t *cp, unsigned long address);
int cache_access(cache_t *cp, unsigned long *up_access_address, unsigned long address, int access_type /*0 for read, 1 for write*/);
//returns 0 (if a hit), 1 (if a miss but no dirty block is writen back) or
//2 (if a miss and a dirty block is writen back)

int cache_retrieve_block(cache_t *cp, struct cache_blk_t **block, unsigned long address);
void cache_block_init(cache_t *cp, struct cache_blk_t *block, unsigned long address);
void cache_block_copy(struct cache_blk_t *dst, struct cache_blk_t *src);
#endif // _CACHE_H

