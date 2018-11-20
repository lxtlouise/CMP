#ifndef _LOG_H
#define _LOG_H
#include "cmp.h"
void log_L1_hit(mem_access_t *access);
void log_L1_miss(mem_access_t *access);
void log_L2_hit(mem_access_t *access);
void log_L2_miss(mem_access_t *access);
void log_L1_write_back(mem_access_t *access);
void log_L2_write_back(mem_access_t *access);
void log_L1_evict(mem_access_t *access);
void log_L1_invalidate(mem_access_t *access);
#endif // _LOG_H
