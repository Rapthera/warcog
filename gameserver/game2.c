#include "game2.h"
#include "client.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define update(x) if (ent->update.x == (uint8_t)(gameframe + 1)) ent->update.x++

static bool facing(uint16_t theta, uint16_t angle)
{
    return (abs((int)angle - theta) < 16384);
}

static bool turn(entity_t *ent, uint16_t angle, uint16_t turnrate)
{
    int16_t dif;

    dif = angle - ent->angle;
    if (abs(dif) < turnrate) {
        ent->angle = angle;
        return 1;
    }

    if (dif > 0) {
        ent->angle += turnrate;
    } else {
        ent->angle -= turnrate;
    }

    return 0;
}

static bool _move(entity_t *ent, point p, uint16_t angle, double movespeed, uint16_t turnrate)
{
    int dx, dy;
    double d;

    if (facing(ent->angle, angle)) {
        dx = p.x - ent->pos.x;
        dy = p.y - ent->pos.y;
        d = (int64_t)dx * dx + (int64_t)dy * dy;

        if (d <= movespeed * movespeed) {
            ent->pos = p;
            return 1;
        }
        ent->pos = addvec(ent->pos, scale(vec(dx, dy), movespeed / sqrt(d)));
    }

    turn(ent, angle, turnrate);
    return 0;
}

static int target(entity_t *ent, point p, uint32_t range, uint8_t target_type)
{
    const entitydef_t *def;
    int res;
    uint16_t angle;

    if (target_type == target_self)
        return 1;

    def = &entitydef[ent->def];
    angle = getangle(ent->pos, p);

    if (!inrange(ent->pos, p, range)) {
        _move(ent, p, angle, def->movespeed, def->turnrate);
        return -1;
    }

    res = facing(ent->angle, angle);
    turn(ent, angle, def->turnrate);

    return res;
}

void entity_frame(entity_t *ent)
{
    int j, n, res;
    uint8_t order, target_type;
    bool complete, cancel, moving;
    ability_t *a;
    order_t *o;

    const entitydef_t *def;
    const abilitydef_t *adef;
    //const statedef_t *sdef;

    def = &entitydef[ent->def];
    if (def->lockanim)
        return;

    ent->anim.frame++;


    complete = 0; moving = 0;
    for (j = 0; j < ent->norder;) {
        o = &ent->order[j];
        order = o->id;
        target_type = o->target_type;

        //target validation..

        cancel = 0;
        if (!complete) {
            if (order & 128) { /* built in orders */
                switch (order & 127) {
                case 0: /* move */
                    if (target_type != target_ground ||
                        _move(ent, o->p, getangle(ent->pos, o->p), def->movespeed, def->turnrate)) {
                        cancel = 1;
                    } else {
                        moving = 1;
                    }
                    complete = 1;
                    break;
                case 1: /* stop */
                    ent->anim = anim(anim_idle, def->idle_len);
                    ent->update.anim = gameframe;
                    ent->norder = j + 1;
                    cancel = 1;
                    complete = 1;
                    break;
                default:
                    cancel = 1;
                    break;
                }
            } else { /* abilities */
                cancel = 1;

                if (order < ent->nability) {
                    a = &ent->ability[order];
                    adef = &abilitydef[a->def];

                    if ((adef->target & (1 << target_type)) && ability_cancast(ent, a)) {
                        complete = (adef->cast_time != 0 || target_type != target_self);

                        res = target(ent, o->p, o->timer ? adef->maxrange : adef->range, target_type);
                        if (res < 0) {
                            o->timer = 0;
                            cancel = 0;
                            moving = 1;
                        } else {
                            if (res || o->timer) {
                                if (!o->timer && adef->anim_time) {
                                    ent->anim = anim(adef->anim, adef->anim_time);
                                    ent->update.anim = gameframe;
                                }

                                if (o->timer == adef->cast_time) {
                                    target_t t = {.pos = o->p};
                                    ability_onimpact(ent, a, t, target_type);
                                } else {
                                    o->timer++;
                                    cancel = 0;
                                }
                            } else {
                                cancel = 0;
                                moving = 1;
                            }
                        }

                        if (target_type == target_self) {
                        }
                    }
                }
            }
        }

        if (cancel) {
            ent->norder--;
            n = ent->norder - j;
            memmove(&ent->order[j], &ent->order[j + 1], n * sizeof(order_t));
            continue;
        }
        j++;
    }

    if (moving) {
        if (ent->anim.id != anim_walk) {
            ent->anim = anim(anim_walk, def->walk_len);
            ent->update.anim = gameframe;
        }
    } else {
        if (ent->anim.id == anim_walk) {
            ent->anim = anim(anim_idle, def->idle_len);
            ent->update.anim = gameframe;
        }
    }

    if (ent->anim.frame == ent->anim.len) {
        ent->anim.frame = 0;

        if (!ent->norder && ent->anim.id != anim_idle) {
            ent->anim = anim(anim_idle, def->idle_len);
            ent->update.anim = gameframe;
        }
    }
}

static void vision_noteam(entity_t *self)
{
    int i;
    client_t *cl;
    entity_t *ent;
    const entitydef_t *def;

    def = &entitydef[self->def];

    cl = &client[self->owner.id];
    if (cl->status < cl_loaded)
        return;

    client_addentity(cl, id(self), gameframe);

    if (!def->vision)
        return;

    entity_loop(ent, i) {
        if (ent->owner.id == self->owner.id || ent->delete || !entity_visible(ent, self))
            continue;

        /* if (!inrange(self->pos, ent->pos, def->vision))
            continue; */

        client_addentity(cl, id(ent), gameframe);
    }
}

void game_frame(void)
{
    int i, j, k;
    entity_t *ent, *proxy;
    ability_t *a;
    state_t *s;
    mapdata_t *md;

    /* main frame */
    entity_loop(ent, i) {
        entity_onframe(ent);
        entity_frame(ent);
    }

    onframe();

    /* post frame */
    entity_loop(ent, i) {
        if (ent->delete)
            continue;

        if (ent->owner.team == 0xFF) {
            if (ent->owner.id != 0xFF) {
                vision_noteam(ent);
            }
        } else {
            //vision(ent);
        }

    }

    /* end frame */
    nentity = 0;
    for (i = 0; i < maxentity; i++) {
        ent = &entity[i];
        if (ent->def == 0xFFFF)
            continue;

        if (ent->delete) {
            ent->def = 0xFFFF;
            entity_free(ent);
            continue;
        }

        if (ent->proxy != 0xFFFF) {
            proxy = &entity[ent->proxy];
            if (proxy->def == 0xFFFF || proxy->delete) {
                ent->proxy = 0xFFFF;
                ent->update.info = gameframe;
            }
        }

        update(spawn);
        update(info);
        update(pos);
        update(anim);
        update(hp);
        update(mana);
        update(ability);
        update(state);

        for (j = 0; j < ent->nability; j++) {
            a = &ent->ability[j];
            if (a->ent != 0xFFFF && ent(a->ent)->delete)
                a->ent = 0xFFFF;
            if (a->cooldown)
                a->cooldown--;
        }
        for (j = 0; j < ent->nstate; ) {
            s = &ent->state[j];
            if ((s->duration && !--s->duration) || (s->expire)) {
                state_onexpire(ent, s);
                for (k = 0; k < ent->nability; k++) {
                    a = &ent->ability[k];
                    if (a->state != 0xFF) {
                        if (a->state == j) {
                            ability_onstateexpire(ent, a, s);
                            a->state = 0xFF;
                        } else if (a->state > j) {
                            a->state--;
                        }
                    }
                }

                memmove(s, s + 1, (--ent->nstate - j) * sizeof(*s));
                ent->update.state = gameframe;
                continue;
            }
            s->age++;
            j++;
        }

        entity_id[nentity++] = i;
    }

    nmapdata = 0;
    for (i = 0; i < maxmapdata; i++) {
        md = &mapdata[i];
        if (md->frame == (uint8_t)(gameframe + 1))
            md->frame++;

        if (md->data[0] == 0xFF)
            continue;

        mapdata_id[nmapdata++] = i;
    }

    //printf("%u\n", nentity);

    client_frame(gameframe);
    gameframe++;
}

void game_init(void)
{
    int i;
    for (i = 0; i < maxentity; i++)
        entity[i].def = 0xFFFF;

    onstart();
}
