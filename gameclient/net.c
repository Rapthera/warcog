#include "net.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "main.h"
#include "input.h"
#include "audio.h"
#include "entity.h"
#include "map.h"
#include "model.h"
#include "util.h"
#include "player.h"
#include "particle.h"

#include "../gameserver/protocol.h"
#include "../gameserver/mapdef.h"

typedef struct {
    uint8_t team, id;
    uint16_t hero;
    uint8_t color[3];
    uint8_t empty[9];
    uint8_t name[16];
} playerdata_t;

#define uint16(x) (*(uint16_t*)(x))
#define uint32(x) (*(uint32_t*)(x))

int send2(const void *data, int len);
void thread(void (func)(void*), void *args);
void postmessage(uint8_t id, uint8_t v8, uint16_t v16, uint32_t value, void *data);
void quit(void);

#define setdef(x,y) x = tmp, tmp += sizeof(*x) * mi->y

static const void* loadsounds(const void *p, int *plen, int nsound)
{
    void *tmp;
    int i, k, len;
    sound_t sound;

    len = *plen;

    int32_t offsets[nsound + 1];

    k = nsound * 4;
    if ((len -= k) < 0)
        return 0;

    offsets[0] = 0;
    memcpy(&offsets[1], p, k); p += k;

    if (offsets[nsound] < 0 || (len -= offsets[nsound]) < 0)
        return 0;

    for (i = 0; i < nsound; i++) {
        k = offsets[i + 1] - offsets[i];
        if (k < 0 || !(tmp = malloc(k)))
            return 0;

        memcpy(tmp, p, k); p += k;

        sound.format = 0;
        sound.samples = k;
        sound.data = tmp;
        audio_load(&audio, sound, i);
    }

    *plen = len;
    return p;
}

/* TODO: deflate directly into buffers, free gl/audio stuff on failure */
static void load_data(void *arg)
{
    net_t *net = arg;
    void *data;

    data = malloc(net->map_inflated_size);
    if (!data)
        return;

    printf("decompressing...\n");

    assert(inflate(data, net->map_data, net->map_inflated_size, net->map_size) ==
           net->map_inflated_size);
    free(net->map_data); net->map_data = 0;

    printf("loading...\n");

    const void *p = data;
    int len = net->map_inflated_size;

    const mapdef_t *mi;
    void *tmp;
    int i, k;
    uint16_t res[3];

    if ((len -= sizeof(*mi)) < 0)
        goto free_data;

    mi = p; p = (mi + 1);

    /* defs */
    /* assume all def_t need same alignement */
    k = mi->ndef * sizeof(entitydef_t) + mi->nadef * sizeof(abilitydef_t) +
        mi->nsdef * sizeof(statedef_t) + mi->neffect * sizeof(effectdef_t) +
        mi->nparticle * sizeof(particledef_t) + mi->ngs * sizeof(groundspritedef_t) +
        mi->nattachment * sizeof(attachmentdef_t) + mi->nmdleffect * sizeof(mdleffectdef_t) +
        mi->nstrip * sizeof(stripdef_t) + mi->nsoundeff * sizeof(sounddef_t);

    if ((len -= k) < 0 || !(tmp = malloc(k)))
        goto free_data;

    memcpy(tmp, p, k); p += k;
    setdef(entitydef, ndef);
    setdef(abilitydef, nadef);
    setdef(statedef, nsdef);
    setdef(effectdef, neffect);
    setdef(particledef, nparticle);
    setdef(groundspritedef, ngs);
    setdef(attachmentdef, nattachment);
    setdef(mdleffectdef, nmdleffect);
    setdef(stripdef, nstrip);
    setdef(sounddef, nsoundeff);
    enddef = tmp;

    postmessage(msg_loadparticles, 0, mi->nparticle, 0, 0);

    /* text */
    k = mi->textlen;
    if ((len -= k) < 0 || !(tmp = malloc(k)))
        goto free_data;

    memcpy(tmp, p, k); p += k;
    postmessage(msg_loadtext, 0, 0, mi->textlen, tmp);

    /* map */
    k = (6 << (mi->size * 2));
    if ((len -= k) < 0 || !(tmp = malloc(k)))
        goto free_data;

    memcpy(tmp, p, k); p += k;
    postmessage(msg_loadmap, mi->size, 0, 0, tmp);

    /* textures */
    k = (256 * 256 * 4 * mi->ntex) + (256 * 256 * 4) + (1024 * 1024 * 4);
    if ((len -= k) < 0 || !(tmp = malloc(k)))
        goto free_data;

    memcpy(tmp, p, k); p += k;
    postmessage(msg_loadtex, 0, mi->ntex, 0, tmp);

    /* models */
    for (i = 0; i < mi->nmodel; i++) {
        k = model_load(p, res, &len);
        if (!k || !(tmp = malloc(k)))
            goto free_data;

        memcpy(tmp, p, k); p += k;
        postmessage(msg_loadmdl, i, res[0], res[1] | (res[2] << 16), tmp);
    }

    /* sounds */
    p = loadsounds(p, &len, mi->nsound);
    if (!p)
        goto free_data;

    assert(len == 0);
    free(data);

    postmessage(msg_loaded, 0, 0, 0, 0);
    return;

free_data:
    free(data);
    return;
}

static void msg_send(net_t *net, const uint8_t *end)
{
    uint8_t seq, *p;
    int len;

    len = end - net->packet.data;

    send2(&net->packet, 4 + len);
    seq = net->packet.seq++;

    assert(!net->msg[seq].data);

    p = net->msg[seq].data = malloc(len);
    net->msg[seq].len = len;

    assert(p);

    memcpy(p, net->packet.data, len);
}

static void send_ping(void)
{
    printf("pinging server...\n");

    uint8_t id = 0xFF;
    send2(&id, 1);
}

static void send_connect(net_t *net)
{
    uint8_t pk[6 + 16];

    pk[0] = 0xFF;
    memcpy(pk + 1, &net->ip_key, 4);
    memcpy(pk + 5, net->name, 16);
    pk[5 + 16] = net->rate;

    send2(pk, sizeof(pk));
}

#define read(x) memcpy(&x, data, sizeof(x)), data += sizeof(x)

static const uint8_t* readentity(const uint8_t *data)
{
    entity_t *en;
    ability_t *a;
    state_t *s;
    const statedef_t *sdef;
    const entitydef_t *def;
    uint16_t id, d;//, effect;
    uint8_t flags;
    int i;//, j, eid;
    //bool new;

    memcpy(&id, data, 2); data += 2;
    flags = *data++;

    en = ents_get(&ents, id);

    if (!flags) {
        //remove from selection
        entity_clear(en);
        return data;
    }

    if (flags & flag_spawn)
        if (en->def != 0xFFFF)
            entity_clear(en);

    if (flags & flag_info) {
        read(en->def);
        read(en->proxy);
        read(en->team);
        read(en->owner);
    }

    if (flags & flag_pos) {
        read(en->x);
        read(en->y);
        read(en->angle);
    }

    if (flags & flag_anim) {
        read(en->anim_frame);
        read(en->anim);
        read(en->anim_len);
    }

    if (flags & flag_hp) {
        read(en->hp);
    }

    if (flags & flag_mana) {
        read(en->mana);
    }

    if (flags & flag_ability) {
        i = 0;
        while (read(d), d != 0xFFFF) {
            a = &en->ability[i++];
            a->def = d;
            read(a->cooldown);
        }
        en->nability = i;
    }

    if (flags & flag_state) {
        i = 0;
        while (read(d), d != 0xFFFF) {
            s = &en->state[i++];
            s->def = d;
            sdef = &statedef[d];

            read(s->duration);
            read(s->age);

            if (!s->age && sdef->effect != 0xFFFF) {
                s->effect.start = 1;
            }
        }
        en->nstate = i;
    }

    def = &entitydef[en->def];
    if ((flags & flag_spawn) && def->effect != 0xFFFF) {
        en->effect.start = 1;
    }

    return data;
}
#undef read

static void readplayer(const playerdata_t *pd)
{
    player_t *p;

    p = &players.player[pd->id];
    p->status = 1;
    p->team = pd->team;
    memcpy(p->name, pd->name, sizeof(pd->name));
}

static const uint8_t* events(const uint8_t *p)
{
    playerdata_t pd;
    uint8_t ev, len, id;

    while ((ev = *p++) != ev_end) {
        printf("event %u\n", ev);
        switch (ev) {
        case ev_join:
            memcpy(&pd, p, sizeof(pd)); p += sizeof(pd);
            readplayer(&pd);
            players.count++;
            break;
        case ev_leave_timeout ... ev_leave_kicked_removed:
            id = *p++;
            if ((id & 1) == (ev_leave_timeout_removed & 1))
                players.player[id].status = 0, players.count--;
            printf("dc %u\n", id);
            break;
        case ev_servermsg:
            len = *p++;
            printf("svmsg: %.*s\n", len, p);
            p += len;
        }
    }

    return p;
}

static const uint8_t* readmapdata(const uint8_t *p)
{
    int id;

    while ((id = *p++) != 0xFF) {
        printf("... %u %u\n", id, *p);
        if (*p == 0xFF) {
            map_undeform(&map, id);
            p++;
            continue;
        }

        map_deform(&map, id, p);
        p += 10;
    }

    return p;
}

void net_recv(net_t *net, const uint8_t *data, int len)
{
    uint16_t part_id;
    uint8_t seq, flags, *p, *pp, dif;
    const uint8_t *end;
    const playerdata_t *pd;

    if (!net->connected) { /* TODO: what if ... */
        if (len == sizeof(net->ip_key)) {
            net->ip_key = uint32(data);
            send_connect(net);

            printf("server responded, connecting...\n");
            net->timeout = 0.0;
        } else if (len == 12) {
            net->packet.id = data[0];
            net->connected = 1;


            memcpy(&net->map_size, data + 4, 4);
            memcpy(&net->map_inflated_size, data + 8, 4);

            net->map_parts = (net->map_size - 1) / partsize + 1;


            net->map_data = malloc(net->map_parts * partsize);
            net->map_part = calloc(net->map_parts, sizeof(*net->map_part));

            assert(net->map_data && net->map_part);

            net->map_parts_left = net->map_parts;

            printf("connected... map size is %u (%u)->%u\n",
                   net->map_size, net->map_parts, net->map_inflated_size);
            net->timeout = 0.0;
        }

        return;
    }

    assert(len >= 2);

    seq = data[0]; flags = data[1];
    while (net->cseq != seq) {
        assert(net->msg[net->cseq].data);

        free(net->msg[net->cseq].data);
        net->msg[net->cseq].data = NULL;
        net->cseq++;
    }

    if ((flags & CONTROL_BIT_IN) != (net->packet.flags & CONTROL_BIT_IN)) {
        net->packet.flags ^= CONTROL_BIT_IN;

        p = net->packet.data;
        while (seq != net->packet.seq) {
            assert(net->msg[seq].data);

            memcpy(p, net->msg[seq].data, net->msg[seq].len);
            p += net->msg[seq].len;

            free(net->msg[seq].data);
            net->msg[seq].data = NULL;
            seq++;
        }

        net->packet.seq = data[0];
        msg_send(net, p);
    }


    end = data + len;
    data += 2;

    if ((flags & CONTROL_BIT_OUT) != (net->packet.flags & CONTROL_BIT_OUT))
        return;

    switch (flags & MSG_BITS) {
    case MSG_PART:
        net->timeout = 0.0;

        if (!net->map_parts_left)
            return;

        if (flags & OOP_FLAG) { /* wants a list of parts missing */
            if (!net->sent_oop) {
                p = net->packet.data;
                *p++ = MSG_MISSING;
                pp = p++;

                if (net->map_parts_left) {
                    part_id = 0;
                    do {
                        if (!net->map_part[part_id]) {
                            *(uint16_t*)p = part_id;
                            p += 2;
                        }
                    } while (++part_id != net->map_parts && p != net->packet.data + 13 * 2);
                }

                *pp = (p - net->packet.data - 2) / 2;
                msg_send(net, p);
                net->sent_oop = 1;
            }
        } else {
            net->sent_oop = 0;
        }

        if (len == 2)
            return;

        assert(len == 4 + partsize);
        part_id = uint16(data);
        assert(part_id < net->map_parts);
        if (!net->map_part[part_id]) {
            memcpy(net->map_data + part_id * partsize, data + 2, partsize);
            net->map_part[part_id] = 1;
            net->map_parts_left--;
            if (!net->map_parts_left) {
                printf("map download complete! loading...\n");
                thread(load_data, net);
                free(net->map_part); net->map_part = 0;

                p = net->packet.data;
                *p++ = MSG_LOADING;
                msg_send(net, p);
            }
        }
        return;

    /* todo: length checks */
    case MSG_RESET:;
        printf("reset packet!\n");
        net->packet.frame = *data++ + 1;

        net->timeout = 0.0;

        /* players */
        players_clear(&players);

        players.count = *data++;
        for (pd = (void*)data, data = (void*)&pd[players.count]; pd != (void*)data; pd++) {
            readplayer(pd);
        }

        ents_clear(&ents);
        particles_clear(&particles);
        map_reset(&map);

        data = readmapdata(data);

        /* entites */
        while (data < end)
            data = readentity(data);

        assert(data == end);

        ents_netframe(&ents, 0);
        particles_netframe(&particles, 0);
        break;
    case MSG_DELTA:
        printf("delta packet! %u %u\n", *data, net->packet.frame);

        if (*data++ != net->packet.frame) {
            net->packet.flags ^= CONTROL_BIT_OUT;
            send2(&net->packet, 4);
            return;
        }
        dif = (*data++ + 1) - net->packet.frame;
        net->packet.frame += dif;

        /* events */
        data = events(data);

        data = readmapdata(data);

        /* entites */
        while (data < end)
            data = readentity(data);

        assert(data == end);

        ents_netframe(&ents, dif);
        particles_netframe(&particles, dif);
        net->timeout = 0.0;
        break;
    case MSG_FRAME:
        //printf("frame packet! %u %u\n", end - data, data[3]);

        if (*data++ != net->packet.frame) {
            net->packet.flags ^= CONTROL_BIT_OUT;
            send2(&net->packet, 4);
            return;
        }
        net->packet.frame++;

        /* events */
        data = events(data);

        data = readmapdata(data);

        /* entites */
        while (data < end)
            data = readentity(data);

        assert(data == end);

        net->timeout = 0.0;
        ents_netframe(&ents, 1);
        particles_netframe(&particles, 1);
        break;
    }

    input_validsel(&input);
}

void net_frame(net_t *net, double delta)
{
    net->timeout += delta;

    if (!net->connected) {
        if (net->timeout >= 1.0) {
            if (!net->ip_key) {
                send_ping();
            } else {
                send_connect(net);
            }

            net->timeout = 0.0;
        }

        return;
    }

    net->timer += delta;
    if (net->map_parts_left) {
        if (net->timer >= 1.0) {
            send2(&net->packet, 4);

            net->timer -= 1.0;

            if (net->map_parts_left)
                printf("%u / %u parts remain...\n", net->map_parts_left, net->map_parts);
        }
    } else if (net->timer >= 0.125) {
        send2(&net->packet, 4);
        net->timer = 0.0;
    }

    if (net->timeout >= 10.0) {
        net->timeout = 0.0;
        net->connected = 0;

        printf ("timed out..\n");
    }
}

void net_init(net_t *net, uint8_t dlrate, const char *name)
{
    net->rate = dlrate;
    strcpy(net->name, name);

    send_ping();
}

void net_cleanup(net_t *net)
{
    int i;

    for (i = 0; i < 256; i++)
        free(net->msg[i].data);

    free(net->map_data);
    free(net->map_part);
}

void net_loaded(net_t *net)
{
    uint8_t *p;

    p = net->packet.data;
    *p++ = MSG_LOADED;
    msg_send(net, p);

    net->loaded = 1;
}

void net_order(net_t *net, uint8_t action, uint8_t flags, uint16_t id, uint32_t x, uint32_t y)
{
    uint8_t *p;

    p = net->packet.data;
    *p++ = MSG_ORDER;
    *p++ = 1;
    *p++ = action;
    *p++ = flags;
    memcpy(p, &x, 4); p += 4;
    memcpy(p, &y, 4); p += 4;
    memcpy(p, &id, 2); p += 2;

    msg_send(net, p);
}

