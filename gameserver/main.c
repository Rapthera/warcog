#include "main.h"
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "game2.h"
#include "gamedata.h"
#include "ip.h"
#include "protocol.h"

#define slen(x) if ((len -= (x)) < 0) return;

static iptable_t iptable;

const uint32_t colors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF000};

static void setcolor(uint8_t *color)
{
    static int i;

    memcpy(color, &colors[i], 3);
    i = (i + 1) & 3;
}

void on_recv(const uint8_t *data, int len, const addr_t addr)
{
    client_t *cl;
    const ipentry_t *entry;
    uint8_t id, seq, frame, flags, msg, n, i, rep[12];

    slen(1);

    id = *data++;
    if (id == 0xFF) { /* non client message */
        entry = ip_find(&iptable, addr.ip);
        if(!entry)
            return;

        if (!len) {
            sendtoaddr(&entry->key, 4, addr);
            return;
        }

        slen(4 + sizeof(cl->player.name) + 1);

        if (memcmp(data, &entry->key, 4))
            return;

        cl = client_new(addr);
        id = cl_id(cl);

        cl->status = cl_connected;
        cl->timeout = 0;
        cl->seq = 0;
        cl->flags = 0;
        cl->player.id = id;
        setcolor(cl->player.color);
        memcpy(cl->player.name, data + 4, sizeof(cl->player.name));

        cl->dl_rate = data[4 + sizeof(cl->player.name)];
        if (!cl->dl_rate)
            cl->dl_rate = 1;

        if (cl->dl_rate > 10)
            cl->dl_rate = 10;

        cl->dl_x = 0;
        cl->dl_part = 0;
        memset(cl->dl_missing, 0xFF, sizeof(cl->dl_missing));

        rep[0] = id;
        memcpy(rep + 4, &gamedata.size, 4);
        memcpy(rep + 8, &gamedata.inflated, 4);
        sendtoaddr(rep, 12, addr);
        return;
    }

    cl = &client[id];
    if (cl->status <= cl_disconnected || !addr_eq(cl->addr, addr))
        return;

    slen(3);
    seq = *data++; frame = *data++; flags = *data++;

    if ((flags & CONTROL_BIT_OUT) != (cl->flags & CONTROL_BIT_OUT)) {
        if (cl->status == cl_loaded)
            cl->status = cl_loaded_delta;
        cl->flags ^= CONTROL_BIT_OUT;
        cl->delta_frame = frame;
    }

    if ((flags & CONTROL_BIT_IN) != (cl->flags & CONTROL_BIT_IN))
        return;

    if (seq != cl->seq) {
        cl->flags ^= CONTROL_BIT_IN;
        return;
    }

    cl->timeout = 0;
    slen(1);
    cl->seq++;

    /* handle message */
    do {
        msg = *data++;
        switch (msg) {
        case MSG_MISSING:
            slen(1);
            n = *data++;

            if (!n || n > 12)
                return;

            slen(n * 2);
            i = 0;
            do {
                memcpy(&cl->dl_missing[i++], data, 2);
                data += 2;
            } while (--n);

            cl->flags &= (~OOP_FLAG);
            break;
        case MSG_LOADING:
            if (cl->status == cl_connected)
                cl->status = cl_loading;
            break;
        case MSG_LOADED:
            if (cl->status == cl_loading) {
                cl->status = cl_loaded_reset;
                client_join(cl);
            }
            break;
        case MSG_CHAT:
            slen(1);
            n = *data++;

            if (n > 128)
                return;

            slen(n);

            //chat(cl, data, n)

            data += n;
            break;
        case MSG_ORDER:;
            uint8_t flags;
            uint16_t ent_id;
            order_t order;
            entity_t *ent;

            slen(10);
            n = *data++;
            order.id = *data++;
            flags = *data++;

            order.target_type = flags & 15;
            order.timer = 0;
            memcpy(&order.p, data, sizeof(order.p)); data += sizeof(order.p);

            slen(n * 2);
            do {
                memcpy(&ent_id, data, 2); data += 2;
                ent = &entity[ent_id];

                if (ent->def == 0xFFFF || ent->owner.id != id || !entitydef[ent->def].control)
                    continue;

                if (flags & 32) {
                    memmove(&ent->order[1], &ent->order[0], 7 * sizeof(order_t));
                    ent->order[0] = order;
                    if (ent->norder < 8)
                        ent->norder++;
                } else {
                    if (flags & 16)
                        i = ((ent->norder != 8) ? ent->norder : 7);
                    else
                        i = (ent->norder && ent->order[0].timer != 0);

                    ent->order[i] = order;
                    ent->norder = i + 1;
                }
            } while (--n);

            break;
        default:
            return;
        }
    } while (--len >= 0);
}

void on_frame(void)
{
    game_frame();
}

bool on_init(void)
{
    if (!gamedata_init())
        return 0;

    entity_init();
    mapdata_init();
    client_init();
    game_init();

    return 1;
}
