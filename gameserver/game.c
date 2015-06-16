#include "game.h"
#include "client.h"
#include <stdio.h>
#include <string.h>

#define update(ent, x) ent->update.x = gameframe

void teleport(entity_t *ent, point pos)
{
    ent->pos = pos;
    update(ent, pos);
}

void move(entity_t *ent, vec v)
{
    ent->pos = addvec(ent->pos, v);
    update(ent, pos);
}

void setfacing(entity_t *ent, angle_t angle)
{
    ent->angle = angle;
    update(ent, pos);
}

void sethp(entity_t *ent, uint32_t hp)
{
    ent->hp = hp;
    update(ent, hp);
}

void setmana(entity_t *ent, uint32_t mana)
{
    ent->mana = mana;
    update(ent, mana);
}

void setmodifier(entity_t *ent, modifier_t modifier)
{
    ent->modifiers |= modifier;
}

void removemodifier(entity_t *ent, modifier_t modifier)
{
    ent->modifiers &= (~modifier);
}

bool hasmodifier(entity_t *ent, modifier_t modifier)
{
    return ((ent->modifiers & modifier) != 0);
}

void refresh(entity_t *ent)
{
    const entitydef_t *def;
    ability_t *a;
    int i;

    def = &entitydef[ent->def];
    ent->hp = def->hp;
    ent->mana = 0;

    for (i = 0; i < ent->nability; i++) {
        a = &ent->ability[i];
        if (a->cooldown) {
            a->cooldown = 0;
            update(a, cooldown);
        }
    }

    update(ent, hp);
    update(ent, mana);
    update(ent, ability);
}

void setproxy(entity_t *ent, entity_t *proxy)
{
    ent->proxy = id(proxy);
    update(ent, info);
}

void damage(entity_t *ent, entity_t *source, uint32_t amount)
{
    if (!ent->hp)
        return;

    if (ent->hp <= amount) {
        ent->hp = 0;
        entity_ondeath(ent);
    } else {
        ent->hp -= amount;
    }

    update(ent, hp);
}

void givemana(entity_t *ent, uint32_t amount)
{
    const entitydef_t *def;

    def = &entitydef[ent->def];
    ent->mana += amount;
    if (ent->mana > def->mana)
        ent->mana = def->mana;

    update(ent, mana);
}

void removemana(entity_t *ent, uint32_t amount)
{
    if (!ent->mana)
        return;

    if (ent->mana <= amount)
        ent->mana = 0;
    else
        ent->mana -= amount;

    update(ent, mana);
}


void delete(entity_t *ent)
{
    ent->delete = 1;
}

state_t* applystate(entity_t *ent, uint16_t id, uint16_t duration)
{
    state_t *s;

    if (ent->nstate == 32)
        return 0;

    s = &ent->state[ent->nstate++];
    s->def = id;
    s->duration = duration;
    s->age = -1;
    s->expire = 0;

    update(s, spawn);
    update(ent, state);

    return s;
}

ability_t* giveability(entity_t *ent, uint16_t id)
{
    ability_t *a;

    if (ent->nability == 16)
        return 0;

    a = &ent->ability[ent->nability++];
    a->def = id;
    a->state = 0xFF;

    update(a, spawn);
    update(ent, ability);

    return a;
}

void startcooldown(entity_t *ent, ability_t *a, uint16_t cooldown)
{
    a->cooldown = cooldown;
    update(a, cooldown);
    update(ent, ability);
}

void setabilitystate(entity_t *ent, ability_t *a, entity_t *sent, state_t *s)
{
    if (ent != sent) {
        printf("warning setabilitystate()\n");
        return;
    }

    a->state = (s - ent->state);
}

void changeability(entity_t *ent, ability_t *a, uint16_t def)
{
    a->def = def;
    update(a, info);
    update(ent, ability);
}

void expirestate(entity_t *ent, uint16_t id)
{
    int i;
    for (i = 0; i < ent->nstate; i++)
        if (ent->state[i].def == id)
            ent->state[i].expire = 1;
}

state_t* getstate(entity_t *ent, uint16_t id)
{
    int i;
    for (i = 0; i < ent->nstate; i++)
        if (ent->state[i].def == id)
            return &ent->state[i];

    return 0;
}

void expire(entity_t *ent, state_t *s)
{
    s->expire = 1;
}

/* TODO: pass length instead of using strlen */
void msgplayer(player_t *player, const char *msg)
{
    client_t *cl;
    uint8_t *data;
    int len;

    cl = &client[player->id];
    len = strlen(msg);
    data = event_write(&cl->ev, gameframe, 2 + len);
    if (!data)
        return;

    data[0] = ev_servermsg;
    data[1] = len;
    memcpy(data + 2, msg, len);
}

void msgall(const char *msg)
{
    int i;
    client_t *cl;

    client_loop(cl, i) {
        if (cl->status != cl_connected)
            continue;

        msgplayer(&cl->player, msg);
    }
}

entity_t *spawnunit2(uint16_t def_id, point pos, angle_t angle, owner_t owner)
{
    entity_t *ent;
    const entitydef_t *def;

    ent = entity_new();
    def = &entitydef[def_id];

    memset(ent, 0, sizeof(*ent));
    ent->def = def_id;
    ent->proxy = 0xFFFF;
    ent->pos = pos;
    ent->angle = angle;
    ent->owner = owner;

    ent->anim = anim(anim_idle, def->idle_len);

    ent->hp = def->hp;
    ent->mana = 0;//def->mana;


    ent->update.spawn = gameframe;

    entity_id[nentity++] = id(ent);

    return ent;
}

entity_t *spawnunit(uint16_t def_id, entity_t *owner)
{
    entity_t *ent;

    ent = spawnunit2(def_id, owner->pos, owner->angle, owner->owner);
    ent->modifiers = owner->modifiers;
    return ent;
}

uint8_t deform(point min, point max, int8_t height)
{
    mapdata_t *md;

    md = mapdata_new();
    if (!md)
        return 0xFF;

    md->frame = gameframe;
    md->data[0] = 0;
    md->data[1] = height;
    md->x = min.x;
    md->y = min.y;
    md->x2 = max.x;
    md->y2 = max.y;

    return md_id(md);
}

void deform_undo(uint8_t id)
{
    if (id == 0xFF)
        return;

    mapdata[id].frame = gameframe;
    mapdata_free(id);
}

uint8_t water(point min, point max, uint8_t height)
{
    mapdata_t *md;

    md = mapdata_new();
    if (!md)
        return 0xFF;

    md->frame = gameframe;
    md->data[0] = 128;
    md->data[1] = height;
    md->x = min.x;
    md->y = min.y;
    md->x2 = max.x;
    md->y2 = max.y;

    return md_id(md);
}

