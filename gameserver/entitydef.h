#include <stdint.h>

typedef struct {
    uint16_t start;
    uint8_t len;
} voice_t;

enum {
    ui_selectable = 1,
};

typedef struct {
    uint8_t group, control, target, lockanim;

    double movespeed;
    uint32_t vision;
    uint16_t turnrate;

    uint32_t hp, mana;

    uint16_t idle_len, walk_len;
    /* used only by client*/
    uint16_t model;
    uint8_t anim[32];

    uint16_t effect;

    voice_t voice[2];

    uint8_t uiflags;
    float boundsradius, boundsheight, modelscale;
} entitydef_t;

#define voicerange(x, y) {x, (y) - (x)}
