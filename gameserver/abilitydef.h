#include <stdint.h>

enum {
    tf_self = 1,
    tf_ground = 2,
    tf_unit = 4,
};

enum {
    target_self,
    target_ground,
    target_unit,
};

typedef struct {
    uint16_t icon;
    uint8_t key, queue;

    uint32_t name, description, extra;

    uint8_t anim;
    uint8_t target;
    uint16_t anim_time;
    uint16_t cast_time, cooldown;
    uint32_t range, maxrange;
} abilitydef_t;
