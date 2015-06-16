#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include "core.h"

typedef struct {
    uint16_t def, cooldown;
    struct {
        uint8_t spawn, info, cooldown;
    } update;

    /* */
    uint16_t ent;
    uint8_t state;
} ability_t;

typedef struct {
    uint16_t def, duration, age;
    struct {
        uint8_t spawn, duration;
    } update;

    /* */
    bool expire;

    point point[1];
    uint32_t ivar[3];
    double var[2];
    vec vec[1];
} state_t;

typedef struct {
    bool delete;
    uint8_t tmp;

    uint16_t def;
    uint16_t proxy;
    owner_t owner;
    point pos;
    angle_t angle;
    //uint16_t anim_len;
    anim_t anim;
    uint8_t status, modifiers;
    uint32_t hp, mana;

    struct {
        uint8_t spawn, info, pos, anim, hp, mana, ability, state;
    } update;

    /* */
    uint8_t norder, nability, nstate;
    order_t order[8];
    ability_t ability[16];
    state_t state[32];

    /* */
    vec vec[1];
    double var[1];
    int ivar[1];
} entity_t;

#define maxentity 65535
entity_t entity[65535];
uint16_t entity_id[65535], nentity;

entity_t* entity_new(void);
void entity_free(entity_t *ent);
void entity_init(void);
#define id(ent) (int)((ent) - entity)
#define ent(id) (&entity[id])

#define entity_loop(ent, i) i = 0; while (ent = &entity[entity_id[i]], ++i <= nentity)

#endif

