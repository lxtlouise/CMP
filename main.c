#include <stdio.h>
#include <stdlib.h>
#include "DataTypes.h"
int main()
{
    read_configfile("config.txt");
    init_cpu();
    struct cache_t *c = cache_create(8, 64, 2);

    return 0;
}
