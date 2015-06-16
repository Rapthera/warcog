#ifndef NET_H
#define NET_H

#include <stdint.h>

typedef struct {
    uint16_t family, port;
    uint32_t ip;
    uint8_t padding[8];
} addr_t;

#define addr_eq(a, b) ((a).family == (b).family && (a).port == (b).port && (a).ip == (b).ip)

/* implemented by platform-specific  */
int sendtoaddr(const void *data, int len, const addr_t addr);

#endif
