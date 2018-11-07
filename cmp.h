#ifndef _CMP_H
#define _CMP_H
#include "DataTypes.h"


int tile2tile_msg(Tile *src, Tile *dst); /* returns the delay for tile to tile message */

void process_issued_requests(memory_request_t *requests, int n_requests);

#endif // _CMP_H
