#include "mapdata.h"

static int nfree;
static uint8_t free_id[maxmapdata];

void mapdata_init(void)
{
    int i;
    for (i = 0; i < maxmapdata; i++) {
        free_id[i] = maxmapdata - (i + 1);
        mapdata[i].data[0] = 0xFF;
    }

    nfree = maxmapdata;
}

mapdata_t* mapdata_new(void)
{
    if (!nfree)
        return 0;

    return &mapdata[free_id[--nfree]];
}

void mapdata_free(uint8_t id)
{
    mapdata[id].data[0] = 0xFF;
    free_id[nfree++] = id;
}
