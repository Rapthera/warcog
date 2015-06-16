#include "particle.h"

#include "entity.h"

void particles_new(particles_t *parts, uint32_t x, uint32_t y, uint32_t z, uint8_t d)
{
    particle_t *p;
    const particledef_t *def;

    if (parts->count == 256)
        return;

    p = &parts->p[parts->count++];
    def = &particledef[d];
    p->x = x; p->y = y; p->z = z;
    p->def = d;
    p->lifetime = def->lifetime;
}

void particles_netframe(particles_t *parts, uint8_t delta)
{
    int i, j;
    particle_t *p;
    //const particledef_t *def;

    for (j = 0, i = 0; i < parts->count; i++) {
        p = &parts->p[i];
        //def = &particledef[p->def];

        if (p->lifetime <= delta)
            continue;
        p->lifetime -= delta;

        parts->p[j++] = *p;
    }
    parts->count = j;
}

void particles_clear(particles_t *parts)
{
    parts->count = 0;
}
