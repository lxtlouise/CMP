#include "DataTypes.h"

cache_t * cache_create(int size, int blocksize, int assoc)
{
  int i, nblocks , nsets , k;
  cache_t *C = (cache_t *)calloc(1, sizeof(cache_t));

  nblocks = size *1024 / blocksize ;// number of blocks in the cache
  nsets = nblocks / assoc ;			// number of sets (entries) in the cache
  C->blocksize = blocksize ;
  C->nsets = nsets  ;
  C->assoc = assoc;

  C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

  for(i = 0; i < nsets; i++) {
		C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
		for (k=0 ; k< assoc ; k++){
            C->blocks[i][k].directory_entry.owner_tiles = (int*)calloc(cpu.n_tiles, sizeof(int));
		}
	}
  return C;
}

void updateLRU(cache_t *cp ,int index, int way)
{
int k ;
for (k=0 ; k< cp->assoc ; k++)
{
  if(cp->blocks[index][k].LRU < cp->blocks[index][way].LRU)
     cp->blocks[index][k].LRU = cp->blocks[index][k].LRU + 1 ;
}
cp->blocks[index][way].LRU = 0 ;
}

unsigned long cache_getAddress(cache_t *cp, unsigned long set_index, unsigned long tag){
    //block_address = (address / cp->blocksize);
    //tag = block_address / cp->nsets;
    //index = block_address - (tag * cp->nsets) ;

    unsigned long block_address = set_index + tag*cp->nsets;

    unsigned long address = block_address * cp->blocksize;
    return address;
}

unsigned long cache_get_block_address(cache_t *cp, unsigned long address){
    unsigned long block_address = (address / cp->blocksize);
    return block_address * cp->blocksize;
}

int cache_access(cache_t *cp, unsigned long *up_access_address, unsigned long address, int access_type /*0 for read, 1 for write*/)
//returns 0 (if a hit), 1 (if a miss but no dirty block is writen back) or
//2 (if a miss and a dirty block is writen back)
{
int i ;
int block_address ;
int index ;
int tag ;
int way ;
int max ;

block_address = (address / cp->blocksize);
tag = block_address / cp->nsets;
index = block_address - (tag * cp->nsets) ;

for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
  if (cp->blocks[index][i].tag == tag && cp->blocks[index][i].valid == 1)
  {
	updateLRU(cp, index, i) ;
	if (access_type == 1) cp->blocks[index][i].dirty = 1 ;
	return(0);						/* a cache hit */
  }
}
/* a cache miss */
for (way=0 ; way< cp->assoc ; way++)		/* look for an invalid entry */
    if (cp->blocks[index][way].valid == 0)	/* found an invalid entry */
	{
      cp->blocks[index][way].valid = 1 ;
      cp->blocks[index][way].tag = tag ;
	  cp->blocks[index][way].block_address = cache_getAddress(cp, index, tag);
	  updateLRU(cp, index, way);
	  cp->blocks[index][way].dirty = 0;
      if(access_type == 1) cp->blocks[index][way].dirty = 1 ;
	  return(1);
    }

 max = cp->blocks[index][0].LRU ;			/* find the LRU block */
 way = 0 ;
 for (i=1 ; i< cp->assoc ; i++)
  if (cp->blocks[index][i].LRU > max) {
    max = cp->blocks[index][i].LRU ;
    way = i ;
  }
// cp->blocks[index][way] is the LRU block
up_access_address = cp->blocks[index][way].block_address;
cp->blocks[index][way].tag = tag;
cp->blocks[index][way].block_address = cache_getAddress(cp, index, tag);
updateLRU(cp, index, way);
if (cp->blocks[index][way].dirty == 0)
  {											/* the evicted block is clean*/
	cp->blocks[index][way].dirty = access_type;
	return(1);
  }
else
  {											/* the evicted block is dirty*/
	cp->blocks[index][way].dirty = access_type;
	return(2);
  }

}

int cache_retrieve_block(cache_t *cp, struct cache_blk_t **block, unsigned long address){
    int i ;
    int block_address ;
    int index ;
    int tag ;
    int way ;
    int max ;

    block_address = (address / cp->blocksize);
    tag = block_address / cp->nsets;
    index = block_address - (tag * cp->nsets) ;

    for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
        if (cp->blocks[index][i].tag == tag) // found block for the address
        {
            *block = &(cp->blocks[index][i]);
            return CACHE_SAME_BLOCK;
        }
    }

    for (way=0 ; way< cp->assoc ; way++)		/* look for an invalid entry */
        if (cp->blocks[index][way].valid == 0)	/* found an invalid entry */
        {
            *block = &(cp->blocks[index][way]);
            return CACHE_DIFFERENT_BLOCK;
        }

    max = cp->blocks[index][0].LRU ;			/* find the LRU block */
    way = 0 ;
    for (i=1 ; i< cp->assoc ; i++)
        if (cp->blocks[index][i].LRU > max) {
            max = cp->blocks[index][i].LRU ;
            way = i ;
        }
    *block = &(cp->blocks[index][way]);
    return CACHE_DIFFERENT_BLOCK;
}

void cache_apply_access(cache_t *cp, struct cache_blk_t *block, unsigned long address, int access_type /*0 for read, 1 for write*/)
{
    int i ;
    int block_address ;
    int index ;
    int tag ;
    int way ;
    int max ;

    block_address = (address / cp->blocksize);
    tag = block_address / cp->nsets;
    index = block_address - (tag * cp->nsets) ;

    for (i = 0; i < cp->assoc; i++) {	/* look for the requested block */
        if (&(cp->blocks[index][i]) == block)
        {
            updateLRU(cp, index, i) ;
            cp->blocks[index][way].valid = 1 ;
            cp->blocks[index][way].tag = tag ;
            cp->blocks[index][way].block_address = cache_getAddress(cp, index, tag);
            if (access_type == 1) cp->blocks[index][i].dirty = 1 ;
        }
    }
}

void cache_block_init(cache_t *cp, struct cache_blk_t *block, unsigned long address){
    int i;
    unsigned long block_address = (address / cp->blocksize);
    unsigned long tag = block_address / cp->nsets;
    //index = block_address - (tag * cp->nsets) ;
    block->valid = 1 ;
    block->tag = tag ;
    block->block_address = cache_get_block_address(cp, address);
    block->directory_entry.block_state = BLOCK_I;
    for(i=0; i<cpu.n_tiles; i++){
        block->directory_entry.owner_tiles[i] = 0;
    }

}

void cache_block_copy(struct cache_blk_t *dst, struct cache_blk_t *src){
    dst->block_address = src->block_address;
    dst->directory_entry.block_state = src->directory_entry.block_state;
    memcpy(dst->directory_entry.owner_tiles, src->directory_entry.owner_tiles, sizeof(int) * cpu.n_tiles);
    dst->tag = src->tag;
}
