#include <stdint.h>
#include "math.h"
#include "../gameserver/effectdef.h"

typedef struct {
    uint32_t x, y, z;
    uint8_t def, unused;
    uint16_t lifetime;
} particle_t;

typedef struct {
    int count;
    particle_t p[256];
} particles_t;

particles_t particles;

void particles_new(particles_t *parts, uint32_t x, uint32_t y, uint32_t z, uint8_t d);
void particles_netframe(particles_t *parts, uint8_t delta);

void particles_clear(particles_t *parts);
