#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include "../gameserver/entitydef.h"
#include "../gameserver/abilitydef.h"
#include "../gameserver/statedef.h"
#include "../gameserver/effectdef.h"

typedef struct {
    bool start;
    int var[4];
} effect_t;

typedef struct {
    uint16_t def, cooldown;
    effect_t effect;
} ability_t;

typedef struct {
    uint16_t def, duration, age;
    effect_t effect;
} state_t;

typedef struct {
    uint16_t def, proxy;
    uint8_t team, owner;

    uint32_t x, y;
    uint16_t angle;

    uint16_t anim_frame;
    uint8_t anim, anim_len;

    uint16_t hp, mana;

    uint8_t modifiers;

    vec3 pos;

    effect_t effect;
    int voicesound;

    uint8_t nability, nstate;
    ability_t ability[16];
    state_t state[16];
} entity_t;

typedef struct {
    uint16_t count, list[65535];
    entity_t ent[65536];
} ents_t;

ents_t ents;

const entitydef_t *entitydef;
const abilitydef_t *abilitydef;
const statedef_t *statedef;
const effectdef_t *effectdef;
const particledef_t *particledef;
const groundspritedef_t *groundspritedef;
const attachmentdef_t *attachmentdef;
const mdleffectdef_t *mdleffectdef;
const stripdef_t *stripdef;
const sounddef_t *sounddef;
const void *enddef;

vec3 entity_bonepos(const entity_t *ent, uint8_t bone);
/* call when an entity is removed/replaced */
void entity_clear(entity_t *ent);

/* get entity from id */
entity_t* ents_get(ents_t *ents, int id);
/* call for each net frame received
 reset: delta = 0, frame: delta = 1, delta: delta > 1*/
void ents_netframe(ents_t *ents, uint8_t delta);
/* call before drawing
 delta = time since last net frame */
void ents_gfxframe(ents_t *ents, double delta);
/* call when receiving reset packet or on exit */
void ents_clear(ents_t *ents);
/* init, assumes zeroed-out */
void ents_init(ents_t *ents);

#endif
