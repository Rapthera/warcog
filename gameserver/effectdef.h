#ifndef EFFECTDEF_H
#define EFFECTDEF_H

#include <stdint.h>

enum {
    effect_particles,
    effect_strip,
    effect_sound,
    effect_groundsprite,
    effect_mdl,
    effect_attachment,
};

typedef struct {
    uint8_t bone, rate, start_frame, end_frame;
    uint8_t color[4];
    uint16_t texture, lifetime;
    float start_size, end_size, speed;
    float x, y, z;
} particledef_t;

typedef struct {
    uint8_t start_height, end_height, texture, size;
    uint16_t turnrate, lifetime;
} groundspritedef_t;

typedef struct {
    uint8_t bone, unused;
    uint16_t model;
    float size, x, y, z;
} attachmentdef_t;

typedef struct {
    uint16_t texture;
    uint8_t color[4];
} mdleffectdef_t;

typedef struct {
    uint8_t bone1, bone2;
    uint16_t texture, lifetime;
    float size, speed;
} stripdef_t;

typedef struct {
    uint8_t loop, unused;
    uint16_t sound;
} sounddef_t;

typedef struct {
    uint8_t type, i;
} effectid_t;

typedef struct {
    uint8_t count;
    effectid_t id[4];
    //uint16_t duration;

} effectdef_t;

#endif
