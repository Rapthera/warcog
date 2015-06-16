#ifndef GAME_H
#define GAME_H

#include "entity.h"
#include "mapdata.h"

#include "entitydef.h"
#include "abilitydef.h"
#include "statedef.h"
#include "effectdef.h"
#include "mapdef.h"

enum {
    anim_idle,
    anim_walk,
    anim_attack,
    anim_spell1,
    anim_spell2,
    anim_spell3,
    anim_spell4,
};

#define mdl_none 0xFFFF

#define mapdef_defaults .ndef = nentitydef, .nadef = nabilitydef, .nsdef = nstatedef, \
    .neffect = neffectdef, .nparticle = nparticledef, .ngs = ngsdef, .nattachment = nattachmentdef, \
    .nmdleffect = nmdleffectdef, .nstrip = nstripdef, .nsoundeff = nsounddef, \
    .ntex = num_texture, .nmodel = num_model, .nsound = num_sound, \
    .size = mapshift, .textlen = text_len

#define mapsize (1 << mapshift)

typedef struct {
    uint8_t tile[mapsize][mapsize];
    uint8_t height[mapsize][mapsize][4];
    uint8_t path[mapsize][mapsize];
} map_t;

typedef uint8_t modifier_t;

extern const entitydef_t entitydef[];
extern const abilitydef_t abilitydef[];
extern const statedef_t statedef[];

extern const effectdef_t effectdef[];
extern const particledef_t particledef[];
extern const groundspritedef_t groundspritedef[];
extern const attachmentdef_t attachmentdef[];
extern const mdleffectdef_t mdleffectdef[];
extern const stripdef_t stripdef[];
extern const sounddef_t sounddef[];

extern const char text[];

extern const mapdef_t mapdef;

struct {
    uint16_t len;
    uint16_t ent[65535];
} list;

uint32_t gameframe;
map_t map;


void teleport(entity_t *ent, point pos);
void move(entity_t *ent, vec v);
void setfacing(entity_t *ent, angle_t angle);
void sethp(entity_t *ent, uint32_t hp);
void setmana(entity_t *ent, uint32_t mana);

void setmodifier(entity_t *ent, modifier_t modifier);
void removemodifier(entity_t *ent, modifier_t modifier);
bool hasmodifier(entity_t *ent, modifier_t modifier);
void refresh(entity_t *ent);
void setproxy(entity_t *ent, entity_t *proxy);
void damage(entity_t *ent, entity_t *source, uint32_t amount);
void givemana(entity_t *ent, uint32_t amount);
void removemana(entity_t *ent, uint32_t amount);
void delete(entity_t *ent);
state_t* applystate(entity_t *ent, uint16_t id, uint16_t duration);
ability_t* giveability(entity_t *ent, uint16_t id);
void startcooldown(entity_t *ent, ability_t *a, uint16_t cooldown);

void setabilitystate(entity_t *ent, ability_t *a, entity_t *sent, state_t *s);
void changeability(entity_t *ent, ability_t *a, uint16_t def);

void expirestate(entity_t *ent, uint16_t id);
state_t* getstate(entity_t *ent, uint16_t id);
void expire(entity_t *ent, state_t *s);

void msgplayer(player_t *player, const char *msg);
void msgall(const char *msg);

entity_t *spawnunit(uint16_t def_id, entity_t *owner);
entity_t *spawnunit2(uint16_t def_id, point pos, angle_t angle, owner_t owner);

uint8_t deform(point min, point max, int8_t height);
void deform_undo(uint8_t id);
uint8_t water(point min, point max, uint8_t height);

#define list_start() list.len = 0
#define list_add(ent) list.ent[list.len++] = id(ent)
#define list(ent, i) i = 0; while (ent = &entity[list.ent[i]], ++i <= list.len)

#define areaofeffect(_pos, _distance, _func) \
{ \
    entity_t *ent; \
    int __i; \
    entity_loop(ent, __i) {\
        if (!inrange(_pos,ent->pos,_distance)) continue; \
        _func \
    } \
} \

#define globaleffect(_func) \
{ \
    entity_t *ent; \
    int __i; \
    entity_loop(ent, __i) {\
        _func \
    } \
} \

#define __super(p, f, ...) p##_##f(__VA_ARGS__)
#define _super(p, f, ...) __super(p, f, __VA_ARGS__)
#define super(...) _super(parent, function, __VA_ARGS__)

#define abilitytext(x) .name = x##_name, .description = x##_description, .extra = x##_extra

enum {
    KEY_A = 'A',
    KEY_B = 'B',
    KEY_C = 'C',
    KEY_D = 'D',
    KEY_E = 'E',
    KEY_F = 'F',
    KEY_G = 'G',
    KEY_H = 'H',
    KEY_I = 'I',
    KEY_J = 'J',
    KEY_K = 'K',
    KEY_L = 'L',
    KEY_M = 'M',
    KEY_N = 'N',
    KEY_O = 'O',
    KEY_P = 'P',
    KEY_Q = 'Q',
    KEY_R = 'R',
    KEY_S = 'S',
    KEY_T = 'T',
    KEY_U = 'U',
    KEY_V = 'V',
    KEY_W = 'W',
    KEY_X = 'X',
    KEY_Y = 'Y',
    KEY_Z = 'Z',
};
#define key(x) (KEY_##x - 'A')

void onstart(void);
void onframe(void);

void player_onframe(player_t *player);
void player_onjoin(player_t *player);
bool player_onleave(player_t *player);

void entity_onframe(entity_t *self);
void entity_onspawn(entity_t *self);
void entity_ondeath(entity_t *self);
bool entity_canmove(entity_t *self);
bool entity_visible(entity_t *self, entity_t *ent);

void ability_onimpact(entity_t *self, ability_t *a, target_t t, uint8_t tt);
bool ability_cancast(entity_t *self, ability_t *a);
void ability_onstateexpire(entity_t *self, ability_t *a, state_t *s);

void state_onexpire(entity_t *self, state_t *s);

#endif
