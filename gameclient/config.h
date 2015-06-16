#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int width, height;
    uint8_t windowed, dlrate, unused;
    char name[13];
} config_t;

bool cfg_init(config_t *cfg, char *cfg_str);

#endif
