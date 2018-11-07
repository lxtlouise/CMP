#include "DataTypes.h"
#include "math.h"
Cpu cpu;

void init_tile(Tile *tile){
    tile->L1_cache = cache_create((1 << config.n1) >> 10, 1 << config.b, 1 << config.a1);
    tile->L2_cache = cache_create((1 << config.n2) >> 10, 1 << config.b, 1 << config.a2);
    tile->access_index = 0;
    tile->delay_offset = 0;
    tile->accesses_capacity = 16;
    tile->accesses = malloc(sizeof(mem_access_t) * tile->accesses_capacity);
    tile->n_accesses = 0;
    tile->is_finished = 1;
}

void init_cpu(){
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
        cpu.tiles[i].index_x = x;
        cpu.tiles[i].index_y = y;
        init_tile(&(cpu.tiles[i]));
        x++;
    }
}

void read_trace_file(char *fileName){
    char *line = (char *) malloc (sizeof(char) * MAX_LINE);
    char *tempLine = (char *) malloc (sizeof(char) * MAX_LINE);
    char *temp = (char *) malloc (sizeof(char) * MAX_LINE);
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
        if ((tempLine = strtok(tempLine, WHITE_SPACE)) == NULL || *tempLine == 0) {
            continue;
        }
        char* line_ptr = NULL;
        tempLine = strtok(line, WHITE_SPACE);
        int clock_cycle = atoi(tempLine);
        tempLine = strtok(NULL, WHITE_SPACE);
        int core_id = atoi(tempLine);
        tempLine = strtok(NULL, WHITE_SPACE);
        int access_type = atoi(tempLine);
        tempLine = strtok(NULL, WHITE_SPACE);
        unsigned long address = strtoll(tempLine, NULL, 0);

        Tile *tile = &(cpu.tiles[core_id]);
        if(tile->n_accesses >= tile->accesses_capacity){
            tile->accesses_capacity = tile->accesses_capacity << 1;
            tile->accesses = realloc(tile->accesses, sizeof(mem_access_t)*tile->accesses_capacity);
        }
        tile->accesses[tile->n_accesses].cycle = clock_cycle;
        tile->accesses[tile->n_accesses].core_id = core_id;
        tile->accesses[tile->n_accesses].access_type = access_type;
        tile->accesses[tile->n_accesses].address = address;
        tile->accesses[tile->n_accesses].delay = 0;
        tile->is_finished = 0;
        if(tile->n_accesses>0){
            tile->accesses[tile->n_accesses].delay = clock_cycle - tile->accesses[tile->n_accesses-1].cycle - 1;
        }
        tile->n_accesses++;

        line = (char *) malloc (sizeof(char) * MAX_LINE);
        tempLine = (char *) malloc (sizeof(char) * MAX_LINE);

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
            if(tile->delay_offset==0){
                *completed = &(tile->accesses[tile->access_index-1]);
                // request completed
            }
        }
        return NULL;
    }
    mem_access_t *access = &(tile->accesses[tile->access_index]);
    if(tile->delay_offset>0 || access->delay>0){
        if(tile->delay_offset>0){
            tile->delay_offset--;
            if(tile->delay_offset==0){
                *completed = &(tile->accesses[tile->access_index-1]);
                // request completed
            }
        } else if(access->delay>0)
            access->delay--;
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
    printf("%-3i: ISSUE    %-6i, %s, 0x%08x\n", request->access->core_id, request->access->cycle, request->access->access_type==READ_ACCESS?" READ":"WRITE", request->access->address);
}

void print_mem_complete(memory_request_t *request){
    printf("%-3i: COMPLETE %-6i, %s, 0x%08x\n", request->access->core_id, request->access->cycle, request->access->access_type==READ_ACCESS?" READ":"WRITE", request->access->address);
}

int execute_mem_request(int clock_cycle, int core_id, int access_type, unsigned long memory_address){
    Tile *tile = &(cpu.tiles[core_id]);
    unsigned long up_address;
    int r = cache_access(tile->L1_cache, &up_address, memory_address, access_type);
    if(r==CACHE_HIT){

    } else if(r == CACHE_MISS_NO_EVICT){
    } else if(r == CACHE_MISS_EVICT){
    }
    //mem_request(tile, memory_address, access_type);
}
