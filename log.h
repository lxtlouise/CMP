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
void print_recent_log();
void log_generic(mem_access_t *access, char* message, int delay);

#endif // _LOG_H
