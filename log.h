#ifndef _LOG_H
#define _LOG_H
#include "cmp.h"
typedef struct _log_entry{
    char msg[256];
    int clock;
    struct _log_entry *next;
}log_entry_t;
void init_log_entry(log_entry_t **entry);
extern log_entry_t *log_head;
extern log_entry_t *log_tail;
void print_recent_log();
void log_generic(mem_access_t *access, char* message, int delay);
/*void log_L1_hit(mem_access_t *access, int delay);
void log_L1_miss(mem_access_t *access, int delay);
void log_L2_hit(mem_access_t *access, int delay);
void log_L2_miss(mem_access_t *access, int delay);
void log_L1_write_back(mem_access_t *access, int delay);
void log_L2_write_back(mem_access_t *access, int delay);
void log_L1_evict(mem_access_t *access, int delay);
void log_L1_invalidate(mem_access_t *access, int delay);*/
#endif // _LOG_H
