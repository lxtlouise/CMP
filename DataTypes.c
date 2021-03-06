#include "DataTypes.h"
#include <math.h>
Cpu cpu;
int debug_mode;
void init_tile(Tile *tile){
    tile->L1_cache = cache_create((1 << config.n1) >> 10, 1 << config.b, 1 << config.a1);
    tile->L2_cache = cache_create((1 << config.n2) >> 10, 1 << config.b, 1 << config.a2);
    tile->access_index = 0;
    tile->delay_offset = 0;
    tile->accesses_capacity = 16;
    tile->accesses = malloc(sizeof(mem_access_t) * tile->accesses_capacity);
    tile->n_accesses = 0;
    tile->is_finished = 1;
    tile->cycles_to_finish = 0;
    tile->short_messages = 0;
    tile->long_messages = 0;
}

void init_cpu(){
    cpu.clock = 1;
    log_head = NULL;
    int i;
    cpu.n_tiles = 1 << config.p;
    int w;

    /*for(cpu.width = (int)ceil(sqrt(cpu.n_tiles)); cpu.width>0; cpu.width--)
        if (cpu.n_tiles % cpu.width == 0)
            break;
    cpu.height = cpu.n_tiles / cpu.width;*/
    if(config.p%2==0){
        cpu.width = (int)(round(sqrt(cpu.n_tiles)));
        cpu.height = cpu.width;
    } else {
        cpu.width = (int)(round(sqrt(1 << (config.p-1)))) << 1;
        cpu.height = (int)(round(sqrt(1 << (config.p-1))));
    }
    cpu.tiles = (Tile*)malloc(sizeof(Tile) * cpu.n_tiles);
    int x=0, y=0;
    for(i=0; i<cpu.n_tiles; i++){
        if(x >= cpu.width){
            x = 0;
            y++;
        }
        cpu.tiles[i].index = i;
        cpu.tiles[i].index_x = x;
        cpu.tiles[i].index_y = y;
        cpu.tiles[i].enroute_access = NULL;
        init_tile(&(cpu.tiles[i]));
        x++;
    }
}

void read_trace_file(char *fileName){
    char *line = NULL;//(char *) malloc (sizeof(char) * MAX_LINE);
    char *tempLine = (char *) malloc (sizeof(char) * MAX_LINE);
    char *temp = (char *) malloc (sizeof(char) * MAX_LINE);
    char *tempLine2;
    char *tok;

    size_t len = 0;
    ssize_t read;

    FILE *fp;

    if ((fp = fopen(fileName, "r")) == NULL) {
        perror ("Error openning the trace file...");
        exit (EXIT_FAILURE);
    }

    while ((read = getlinenew(&line, &len, fp)) != -1) { //loop to read file line by line and tokenize
        strcpy (tempLine, line);
        if ((tempLine2 = strtok(tempLine, WHITE_SPACE)) == NULL || *tempLine == 0) {
            continue;
        }
        char* line_ptr = NULL;
        tempLine2 = strtok(line, WHITE_SPACE);
        int clock_cycle = atoi(tempLine2);
        tempLine2 = strtok(NULL, WHITE_SPACE);
        int core_id = atoi(tempLine2);
        tempLine2 = strtok(NULL, WHITE_SPACE);
        int access_type = atoi(tempLine2);
        tempLine2 = strtok(NULL, WHITE_SPACE);
        unsigned long address = strtoll(tempLine2, NULL, 0);

        Tile *tile = &(cpu.tiles[core_id]);
        if(tile->n_accesses >= tile->accesses_capacity-1){
            tile->accesses_capacity = tile->accesses_capacity << 1;
            tile->accesses = realloc(tile->accesses, sizeof(mem_access_t)*tile->accesses_capacity);
        }
        tile->accesses[tile->n_accesses].cycle = clock_cycle;
        tile->accesses[tile->n_accesses].core_id = core_id;
        tile->accesses[tile->n_accesses].access_type = access_type;
        tile->accesses[tile->n_accesses].address = address;
        tile->accesses[tile->n_accesses].request_delay = 0;
        tile->accesses[tile->n_accesses].status = STATUS_TO_START;
        tile->is_finished = 0;
        if(tile->n_accesses>0){
            tile->accesses[tile->n_accesses].request_delay = clock_cycle - tile->accesses[tile->n_accesses-1].cycle - 1;
        } else
            tile->delay_offset = clock_cycle>0?(clock_cycle - 1) : 0;
        tile->n_accesses++;
        free(line);
        line = NULL;
        //line = (char *) malloc (sizeof(char) * MAX_LINE);
        //tempLine = (char *) malloc (sizeof(char) * MAX_LINE);

    }

    if (fp)
        fclose(fp);
}

mem_access_t * get_tile_next_access(Tile *tile, mem_access_t **completed){
    int i;
    *completed = NULL;
    if(tile->is_finished){
        if(tile->delay_offset>0){
            tile->delay_offset--;
            if(tile->delay_offset==0 && tile->access_index>0){
                *completed = &(tile->accesses[tile->access_index-1]);
                // request completed
            }
        }
        return NULL;
    }
    mem_access_t *access = &(tile->accesses[tile->access_index]);
    if(tile->delay_offset>0 || access->request_delay>0){
        if(tile->delay_offset>0){
            tile->delay_offset--;
            if(tile->delay_offset==0 && tile->access_index>0){
                *completed = &(tile->accesses[tile->access_index-1]);
                // request completed
            }
        } else if(access->request_delay>0)
            access->request_delay--;
        return NULL;
    } else {
        tile->access_index++;
        tile->delay_offset = 0;
        if(tile->access_index>=tile->n_accesses)
            tile->is_finished = 1;
        return access;
    }
}

void print_mem_issue(memory_request_t *request){
    printf("%-3i: %-6i %s 0x%08x  %s\n"
        , request->access->core_id, request->access->cycle, request->access->access_type==READ_ACCESS?"R":"W", request->access->address, request->access->status==STATUS_TO_COMPLETE?" ISSUED":"ENROUTE");
    //printf("%-3i: %s    %-6i, %s[%i], 0x%08x\n", request->access->core_id, request->access->status==STATUS_TO_COMPLETE?" ISSUED":"ENROUTE", request->access->cycle, request->access->access_type==READ_ACCESS?" READ":"WRITE", request->delay, request->access->address);
}

void print_mem_complete(memory_request_t *request){
    //log_generic(request->access, "COMPLETED", 0);
    //printf("%-3i: %-6i %s 0x%08x  COMPLETE\n"
    //    , request->access->core_id, request->access->cycle, request->access->access_type==READ_ACCESS?"R":"W", request->access->address);
    //printf("%-3i: COMPLETE %-6i, %s, 0x%08x\n", request->access->core_id, request->access->cycle, request->access->access_type==READ_ACCESS?" READ":"WRITE", request->access->address);
}

