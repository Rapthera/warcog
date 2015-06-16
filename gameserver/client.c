#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "gamedata.h"

static int nfree;
static uint8_t free_id[maxclient];

client_t* client_new(const addr_t addr)
{
    client_t *cl;
    int i;
    uint8_t id;

    client_loop(cl, i) {
        if (!cl->status)
            continue;

        if (addr_eq(cl->addr, addr))
            return cl;
    }

    if (!nfree)
        return 0;

    cl = &client[id = free_id[--nfree]];
    client_id[nclient++] = id;

    memset(cl, 0, sizeof(*cl));
    cl->addr = addr;
    return cl;
}

void client_free(client_t *cl)
{
    cl->status = cl_none;
    free(cl->ents);
    event_free(&cl->ev);
    free_id[nfree++] = cl_id(cl);
}

void client_join(client_t *cl)
{
    client_t *c;
    int i;
    uint8_t *p;

    player_onjoin(&cl->player);

    client_loop(c, i) {
        if (c->status < cl_loaded || c == cl)
            continue;

        p = event_write(&c->ev, gameframe, 1 + sizeof(cl->player));
        *p++ = ev_join;
        memcpy(p, &cl->player, sizeof(cl->player));
    }
}

void client_leave(client_t *cl, uint8_t reason)
{
    client_t *c;
    int i;
    uint8_t *p, res;

    res = player_onleave(&cl->player);
    if (res) {
        client_free(cl);
    } else {
        cl->status = cl_disconnected;
    }

    res |= (reason << 1);

    client_loop(c, i) {
        if (c->status < cl_loaded || c == cl)
            continue;

        p = event_write(&c->ev, gameframe, 2);
        *p++ = ev_leave_timeout + res;
        *p++ = cl_id(cl);
    }
}

void client_addentity(client_t *cl, uint16_t id, uint8_t frame)
{
    /* ASSUME memory allocation is successful: this function cannot fail */
    clentity_t *ents, *p;
    int imin, imax, imid, size;

    ents = cl->ents;
    size = cl->nents;

    imin = 0;
    imax = size - 1;

    if (!size)
        goto insert;

    while (imin < imax) {
        imid = (imin + imax) / 2;

        if (ents[imid].id < id)
            imin = imid + 1;
        else
            imax = imid;
    }

    if (ents[imin].id == id) {
        p = &ents[imin];
        if ((uint8_t)(p->lastseen + 1) != frame)
            p->firstseen = frame;

        p->lastseen = frame;
        return;
    }

    if (ents[imin].id < id)
        imin++;

insert:
    p = malloc ((size + 1) * sizeof(*p));
    memcpy (&p[0], &ents[0], imin * sizeof(*p));
    memcpy (&p[imin + 1], &ents[imin], (size - imin) * sizeof(*p));

    free(ents);
    cl->ents = p;
    cl->nents++;

    p += imin;
    p->id = id;
    p->lastseen = p->firstseen = frame;
}

static uint8_t* player_data(uint8_t *p)
{
    client_t *cl;
    int i;

    client_loop(cl, i) {
        if (cl->status < cl_loaded)
            continue;
        memcpy(p, &cl->player, sizeof(cl->player)); p += sizeof(cl->player);
    }

    return p;
}

#define write(a, x) memcpy(p, &a->x, sizeof(a->x)); p += sizeof(a->x)

static uint8_t* ability_full(uint8_t *p, entity_t *ent)
{
    ability_t *a;
    int i;
    for (i = 0; i < ent->nability; i++) {
        a = &ent->ability[i];

        write(a, def);
        write(a, cooldown);
    }

    *p++ = 0xFF; *p++ = 0xFF;

    return p;
}


static uint8_t* ability_frame(uint8_t *p, entity_t *ent, uint8_t frame)
{
    ability_t *a;
    int i;
    for (i = 0; i < ent->nability; i++) {
        a = &ent->ability[i];

        write(a, def);
        write(a, cooldown);
    }

    *p++ = 0xFF; *p++ = 0xFF;

    return p;
}

static uint8_t* ability_delta(uint8_t *p, entity_t *ent, uint8_t frame, uint8_t dframe)
{
    ability_t *a;
    int i;
    for (i = 0; i < ent->nability; i++) {
        a = &ent->ability[i];

        write(a, def);
        write(a, cooldown);
    }

    *p++ = 0xFF; *p++ = 0xFF;

    return p;
}

static uint8_t* state_full(uint8_t *p, entity_t *ent)
{
    state_t *s;
    int i;
    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];

        write(s, def);
        write(s, duration);
        write(s, age);
    }

    *p++ = 0xFF; *p++ = 0xFF;

    return p;
}


static uint8_t* state_frame(uint8_t *p, entity_t *ent, uint8_t frame)
{
    state_t *s;
    int i;
    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];

        write(s, def);
        write(s, duration);
        write(s, age);
    }

    *p++ = 0xFF; *p++ = 0xFF;

    return p;
}

static uint8_t* state_delta(uint8_t *p, entity_t *ent, uint8_t frame, uint8_t dframe)
{
    state_t *s;
    int i;
    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];

        write(s, def);
        write(s, duration);
        write(s, age);
    }

    *p++ = 0xFF; *p++ = 0xFF;

    return p;
}

#undef write
#define write(x) memcpy(p, &ent->x, sizeof(ent->x)); p += sizeof(ent->x)
#define writevar() memcpy(p, &var, sizeof(var)); p += sizeof(var)

#define writeframe(x) (ent->update.x == frame && (*flags |= flag_##x, 1))
#define writedelta(x) ((uint8_t)(frame - dframe) >= (uint8_t)(frame - ent->update.x) \
                       && (*flags |= flag_##x, 1))

static uint8_t* entity_full(uint8_t *p, entity_t *ent)
{
    uint16_t id, var;

    memcpy(p, (id = id(ent), &id), 2); p += 2;
    *p++ = 0xFF;

    write(def);
    write(proxy);
    write(owner);

    write(pos);
    write(angle);

    write(anim);

    var = (ent->hp >> 16);
    writevar();

    var = (ent->mana >> 16);
    writevar();

    p = ability_full(p, ent);
    p = state_full(p, ent);

    return p;
}

static uint8_t* entity_frame(uint8_t *p, entity_t *ent, uint8_t frame)
{
    uint16_t id, var;
    uint8_t *flags;

    memcpy(p, (id = id(ent), &id), 2); p += 2;
    flags = ((*p = 0), p++);

    if (writeframe(info)) {
        write(def);
        write(proxy);
        write(owner);
    }

    if (writeframe(pos)) {
        write(pos);
        write(angle);
    }

    if (writeframe(anim)) {
        write(anim);
    }

    if (writeframe(hp)) {
        var = (ent->hp >> 16);
        writevar();
    }

    if (writeframe(mana)) {
        var = (ent->mana >> 16);
        writevar();
    }

    if (writeframe(ability)) {
        p = ability_frame(p, ent, frame);
    }

    if (writeframe(state)) {
        p = state_frame(p, ent, frame);
    }

    if (!*flags)
        return p - 3;

    return p;
}

static uint8_t* entity_delta(uint8_t *p, entity_t *ent, uint8_t frame, uint8_t dframe)
{
    uint16_t id, var;
    uint8_t *flags;

    memcpy(p, (id = id(ent), &id), 2); p += 2;
    flags = ((*p = 0), p++);

    if (writedelta(info)) {
        write(def);
        write(proxy);
        write(owner);
    }

    if (writedelta(pos)) {
        write(pos);
        write(angle);
    }

    if (writedelta(anim)) {
        write(anim);
    }

    if (writedelta(hp)) {
        var = (ent->hp >> 16);
        writevar();
    }

    if (writedelta(mana)) {
        var = (ent->mana >> 16);
        writevar();
    }

    if (writedelta(ability)) {
        p = ability_delta(p, ent, frame, dframe);
    }

    if (writedelta(state)) {
        p = state_delta(p, ent, frame, dframe);
    }

    if (!*flags)
        return p - 3;

    return p;
}

static uint8_t* entity_remove(uint8_t *p, entity_t *ent)
{
    uint16_t id;

    memcpy(p, (id = id(ent), &id), 2); p += 2;
    *p++ = 0;

    return p;
}

#undef write
#undef writevar
#undef write_frame
#undef write_delta

static uint8_t* entity_data_reset(client_t *cl, uint8_t *p, uint8_t frame)
{
    /* ASSUME malloc doesnt fail */
    clentity_t *c, *ents, *cc;

    ents = cc = malloc (cl->nents * sizeof(*ents));

    for (c = cl->ents; c != cl->ents + cl->nents; c++) {
        if (c->lastseen == frame)
            p = entity_full(p, &entity[c->id]);

        if ((uint8_t)(frame + 1) == c->firstseen)
            c->firstseen++;

        if ((uint8_t)(frame + 1) == c->lastseen)
            continue;

        *cc++ = *c;
    }

    free(cl->ents);
    cl->ents = ents;
    cl->nents = (cc - ents);

    return p;
}

static uint8_t* entity_data_delta(client_t *cl, uint8_t *p, uint8_t frame, uint8_t dframe)
{
    /* ASSUME malloc doesnt fail */
    clentity_t *c, *ents, *cc;
    entity_t *ent;
    uint8_t d;

    ents = cc = malloc (cl->nents * sizeof(*ents));

    for (c = cl->ents; c != cl->ents + cl->nents; c++) {
        ent = &entity[c->id];
        if (c->lastseen == frame) {
            d = (uint8_t)(frame - dframe);
            if (d >= (uint8_t)(frame - c->firstseen) || d >= (uint8_t)(frame - ent->update.spawn)) {
                p = entity_full(p, ent);
            } else {
                p = entity_delta(p, ent, frame, dframe);
            }
        } else if ((uint8_t)(frame - dframe) >= (uint8_t)(frame - 1 - c->lastseen)) {
            p = entity_remove(p, ent);
        }

        if ((uint8_t)(frame + 1) == c->firstseen)
            c->firstseen++;

        if ((uint8_t)(frame + 1) == c->lastseen)
            continue;

        *cc++ = *c;
    }

    free(cl->ents);
    cl->ents = ents;
    cl->nents = (cc - ents);

    return p;
}

static uint8_t* entity_data_frame(client_t *cl, uint8_t *p, uint8_t frame)
{
    /* ASSUME malloc doesnt fail */
    clentity_t *c, *ents, *cc;

    ents = cc = malloc (cl->nents * sizeof(*ents));

    for (c = cl->ents; c != cl->ents + cl->nents; c++) {
        if (c->lastseen == frame) {
            if (c->firstseen == frame) {
                p = entity_full(p, &entity[c->id]);
            } else {
                p = entity_frame(p, &entity[c->id], frame);
            }
        } else if (c->lastseen == (uint8_t)(frame - 1)){
            p = entity_remove(p, &entity[c->id]);
        }


        if ((uint8_t)(frame + 1) == c->firstseen)
            c->firstseen++;

        if ((uint8_t)(frame + 1) == c->lastseen)
            continue;

        *cc++ = *c;
    }

    free(cl->ents);
    cl->ents = ents;
    cl->nents = (cc - ents);

    return p;
}

static uint8_t* map_data_reset(uint8_t *p)
{
    int i;
    mapdata_t *md;

    mapdata_loop(md, i) {
        *p++ = md_id(md);
        *p++ = md->data[0];
        *p++ = md->data[1];
        memcpy(p, &md->x, 8); p += 8;
    }

    *p++ = 0xFF;

    return p;
}

static uint8_t* map_data_frame(uint8_t *p, uint8_t frame)
{
    int i;
    mapdata_t *md;

    for (i = 0; i < maxmapdata; i++) {
        md = &mapdata[i];
        if (md->frame != frame)
            continue;

        *p++ = md_id(md);
        *p++ = md->data[0];
        if (md->data[0] == 0xFF)
            continue;
        *p++ = md->data[1];
        memcpy(p, &md->x, 8); p += 8;
    }

    *p++ = 0xFF;

    return p;
}

static uint8_t* map_data_delta(uint8_t *p, uint8_t frame, uint8_t dframe)
{
    int i;
    mapdata_t *md;

    for (i = 0; i < maxmapdata; i++) {
        md = &mapdata[i];
        if ((uint8_t)(frame - dframe) < (uint8_t)(frame - md->frame))
            continue;

        *p++ = md_id(md);
        *p++ = md->data[0];
        if (md->data[0] == 0xFF)
            continue;

        *p++ = md->data[1];
        memcpy(p, &md->x, 8); p += 8;
    }

    *p++ = 0xFF;

    return p;
}


static void sendframe(client_t *cl, uint8_t frame)
{
    uint8_t data[65536];

    uint8_t *p, i, ii;
    uint16_t part;

    p = data;
    *p++ = cl->seq;

    if (cl->status < cl_loaded) {
        *p++ = MSG_PART | cl->flags;
        if (cl->status == cl_connected) { /* TODO: rework /fix */
            i = cl->dl_rate;
            if (!cl->dl_x) {
                do {
                    *(uint16_t*)p = cl->dl_part;
                    memcpy(p + 2, gamedata.data + partsize * cl->dl_part, partsize);
                    sendtoaddr(data, 4 + partsize, cl->addr);

                    if (++cl->dl_part == gamedata.num_parts) {
                        cl->dl_x = 1;
                        cl->dl_part = 0;
                        break;
                    }
                } while (--i);
            } else {
                ii = 0;
                do {
                    part = cl->dl_missing[ii];
                    if (part == 0xFFFF)
                        continue;

                    cl->dl_missing[ii] = 0xFFFF;

                    *(uint16_t*)p = part;
                    memcpy(p + 2, gamedata.data + partsize * part, partsize);
                    sendtoaddr(data, 4 + partsize, cl->addr);

                    if (!--i)
                        break;
                } while (++ii < 12);
            }

            if (i == cl->dl_rate) {
                cl->flags |= OOP_FLAG;
                sendtoaddr(data, 2, cl->addr);
            }
            return;
        }
    } else if (cl->status == cl_loaded) {
        *p++ = MSG_FRAME | cl->flags;
        *p++ = frame;
        p = event_copy(&cl->ev, p, frame, frame);
        *p++ = ev_end;
        p = map_data_frame(p, frame);
        p = entity_data_frame(cl, p, frame);
    } else {
        if (cl->status == cl_loaded_delta) {
            *p++ = MSG_DELTA | cl->flags;
            *p++ = cl->delta_frame;
            *p++ = frame;
            p = event_copy(&cl->ev, p, cl->delta_frame, frame);
            *p++ = ev_end;
            p = map_data_delta(p, frame, cl->delta_frame);
            p = entity_data_delta(cl, p, frame, cl->delta_frame);
        } else {
            *p++ = MSG_RESET | cl->flags;
            *p++ = frame;
            *p++ = nclient;
            p = player_data(p);
            p = map_data_reset(p);
            p = entity_data_reset(cl, p, frame);
        }
        cl->status = cl_loaded;
    }
    sendtoaddr(data, p - data, cl->addr);
}

void client_frame(uint8_t frame)
{
    client_t *cl;
    int i;

    client_loop(cl, i) {
        if (cl->status <= cl_disconnected)
            continue;

        cl->timeout++;
        if (cl->timeout == client_timeout) {
            printf("client timed out\n");
            client_leave(cl, cl_timeout);
            continue;
        }

        sendframe(cl, frame);
        event_clear(&cl->ev, frame + 1);
    }

    nclient = 0;
    for (i = 0; i < maxclient; i++) {
        cl = &client[i];
        if (cl->status != cl_none)
            client_id[nclient++] = i;
    }
}

void client_init(void)
{
    int i;
    for (i = 0; i < maxclient; i++)
        free_id[i] = maxclient - (i + 1);

    nfree = maxclient;
}

