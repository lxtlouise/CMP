#ifndef _CMP_H
#define _CMP_H
#include "DataTypes.h"


int tile2tile_delay(Tile *src, Tile *dst); /* returns the delay for tile to tile message */

Tile* get_home_tile(unsigned long address);
void process_issued_requests(memory_request_t *requests, int n_requests);
void process_enroute_request(memory_request_t *request);

#endif // _CMP_H
