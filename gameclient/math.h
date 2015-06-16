#ifndef MATH_H
#define MATH_H

#include <stdint.h>

typedef struct {
    union {
        struct { int16_t x, y; };
        struct { int16_t min, max; };
    };
} ivec2;

typedef struct {
    union {
        struct { int16_t x, y, z; };
    };
} ivec3;

typedef struct {
    ivec2 pos, tex;
} ivert;

typedef struct {
    union {
        struct { float x, y; };
        struct { float min, max; };
        float v[2];
    };
} vec2;

typedef struct {
    union {
        struct { float x, y, z; };
        float v[3];
    };
} vec3;

typedef struct {
    union {
        vec3 xyz;
        struct { float x, y, z, w; };
        float v[4];
    };
} vec4;

typedef struct {
    union {
        float m[4][4];
        vec4 v[4];
    };
} matrix4_t;

typedef struct {
    vec3 pos, x, y, z;
} ref_t;

#define ivec2(x, y) iv2(x, y)
ivec2 iv2(int16_t x, int16_t y);

#define vec2(x, y) v2(x, y)
vec2 v2(float x, float y);
vec2 scale2(vec2 a, float k);
vec2 add2(vec2 a, vec2 b);
vec2 sub2(vec2 a, vec2 b);

#define vec3(x, y, z) v3(x, y, z)
vec3 v3(float x, float y, float z);

float dot3(vec3 a, vec3 b);
vec3 neg3(vec3 a);
vec3 inv3(vec3 a);
vec3 scale3(vec3 a, float k);
vec3 norm3(vec3 a);
vec3 add3(vec3 a, vec3 b);
#define add3_3(a, b, c) add3(add3(a, b), c)
vec3 sub3(vec3 a, vec3 b);

vec3 cross3(vec3 a, vec3 b);
vec3 lerp3(vec3 a, vec3 b, float t);
vec3 qrot3(vec3 a, vec4 q);

#define vec4(x, y, z, w) v4(x, y, z, w)
vec4 v4(float x, float y, float z, float w);

float dot4(vec4 a, vec4 b);
vec4 scale4(vec4 a, float k);
vec4 add4(vec4 a, vec4 b);
vec4 sub4(vec4 a, vec4 b);

vec4 lerp4(vec4 a, vec4 b, float t);
vec4 mul4(vec4 a, vec4 b);

vec3 ref_transform(vec3 pos, ref_t ref);
float ray_intersect_box(vec3 pos, vec3 div, vec3 min, vec3 max);

#endif
