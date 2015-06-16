#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool connected, sent_oop, loaded;

    char name[17];

    uint8_t cseq;

    uint8_t rate; /* download rate */
    uint32_t ip_key; /* ip-specific key */

    double timer, timeout;

    /* map download vars */
    uint16_t map_parts, map_parts_left;
    int map_size, map_inflated_size;
    bool *map_part;
    void *map_data;

    /* packet data */
    struct {
        uint8_t id, seq, frame, flags;
        uint8_t data[4096 - 4];
    } packet;

    /* sent messages history */
    struct {
        uint8_t *data;
        int len;
    } msg[256];
} net_t;

net_t net;
#define self_id() net.packet.id

void net_recv(net_t *net, const uint8_t *data, int len);
void net_frame(net_t *net, double delta);
void net_init(net_t *net, uint8_t dlrate, const char *name);
void net_cleanup(net_t *net);

void net_loaded(net_t *net);

void net_order(net_t *net, uint8_t action, uint8_t flags, uint16_t id, uint32_t x, uint32_t y);

#endif
