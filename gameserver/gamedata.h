#include <stdint.h>
#include <stdbool.h>

/* data that needs to be uploaded to clients when they connect */
struct {
    void *data;
    uint16_t num_parts;
    uint32_t size, inflated;
} gamedata;

/* initialize the mapdata */
bool gamedata_init(void);
