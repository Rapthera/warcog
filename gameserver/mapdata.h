#include <stdint.h>

typedef struct {
    uint8_t frame;
    uint8_t data[7];
    uint16_t x, y, x2, y2;
} mapdata_t;

#define maxmapdata 255
mapdata_t mapdata[255];
uint8_t mapdata_id[255], nmapdata;

#define mapdata_loop(md, i) i = 0; while (md = &mapdata[mapdata_id[i]], ++i <= nmapdata)
#define md_id(md) (int)((md) - mapdata)

void mapdata_init(void);
mapdata_t* mapdata_new(void);
void mapdata_free(uint8_t id);
