#include "math.h"
#include <math.h>
#include <stdbool.h>

ivec2 iv2(int16_t x, int16_t y)
{
    ivec2 res;

    res.x = x;
    res.y = y;

    return res;
}

vec2 v2(float x, float y)
{
    vec2 res;

    res.x = x;
    res.y = y;

    return res;
}

vec2 scale2(vec2 a, float k)
{
    return vec2(a.x * k, a.y * k);
}

vec2 add2(vec2 a, vec2 b)
{
    return vec2(a.x + b.x, a.y + b.y);
}

vec2 sub2(vec2 a, vec2 b)
{
    return vec2(a.x - b.x, a.y - b.y);
}

vec3 v3(float x, float y, float z)
{
    vec3 res;

    res.x = x;
    res.y = y;
    res.z = z;

    return res;
}

float dot3(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 neg3(vec3 a)
{
    return vec3(-a.x, -a.y, -a.z);
}

vec3 inv3(vec3 a)
{
    return vec3(1.0 / a.x, 1.0 / a.y, 1.0 / a.z);
}

vec3 scale3(vec3 a, float k)
{
    return vec3(a.x * k, a.y * k, a.z * k);
}

vec3 norm3(vec3 a)
{
    return scale3(a, 1.0 / sqrt(dot3(a, a)));
}

vec3 add3(vec3 a, vec3 b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

vec3 sub3(vec3 a, vec3 b)
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec3 cross3(vec3 a, vec3 b)
{
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

vec3 lerp3(vec3 a, vec3 b, float t)
{
    return add3(scale3(a, 1.0 - t), scale3(b, t));
}

vec3 qrot3(vec3 a, vec4 q)
{
    return add3(a, scale3(cross3(q.xyz, add3(cross3(q.xyz, a), scale3(a, q.w))), 2.0));
}

vec4 v4(float x, float y, float z, float w)
{
    vec4 res;

    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;

    return res;
}

float dot4(vec4 a, vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

vec4 scale4(vec4 a, float k)
{
    return vec4(a.x * k, a.y * k, a.z * k, a.w * k);
}

vec4 add4(vec4 a, vec4 b)
{
    return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

vec4 sub4(vec4 a, vec4 b)
{
    return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

vec4 lerp4(vec4 a, vec4 b, float t)
{
    vec4 v;

    if (dot4(a, b) > 0.0)
        v = add4(scale4(a, 1.0 - t), scale4(b, t));
    else
        v = sub4(scale4(a, 1.0 - t), scale4(b, t));

    return scale4(v, 1.0 / sqrt(dot4(v, v)));
}

vec4 mul4(vec4 a, vec4 b)
{
    return vec4(
        b.w * a.x + b.x * a.w + b.y * a.z - b.z * a.y,
        b.w * a.y - b.x * a.z + b.y * a.w + b.z * a.x,
        b.w * a.z + b.x * a.y - b.y * a.x + b.z * a.w,
        b.w * a.w - b.x * a.x - b.y * a.y - b.z * a.z
    );
}

vec3 ref_transform(vec3 pos, ref_t ref)
{
    return vec3(dot3(pos, ref.x), dot3(pos, ref.y), dot3(pos, ref.z));
}

/* TODO: cleanup, return "distance" */
float ray_intersect_box(vec3 pos, vec3 div, vec3 min, vec3 max)
{
    float t[2], ty[2], tz[2];
    bool i;

    i = (div.x < 0.0);
    t[i] = (min.x - pos.x) * div.x;
    t[!i] = (max.x - pos.x) * div.x;

    if (div.x == INFINITY) {
        t[0] = (min.x > pos.x) ? INFINITY : -INFINITY;
        t[1] = (max.x > pos.x) ? INFINITY : -INFINITY;
    }

    i = (div.y < 0.0);
    ty[i] = (min.y - pos.y) * div.y;
    ty[!i] = (max.y - pos.y) * div.y;

    if (div.y == INFINITY) {
        ty[0] = (min.y > pos.y) ? INFINITY : -INFINITY;
        ty[1] = (max.y > pos.y) ? INFINITY : -INFINITY;
    }

    if ((t[0] > ty[1]) || (ty[0] > t[1]))
        return INFINITY;

    //res = fmin(t[0], ty[0]);

    if (ty[0] > t[0])
        t[0] = ty[0];
    if (ty[1] < t[1])
        t[1] = ty[1];

    i = (div.z < 0.0);
    tz[i] = (min.z - pos.z) * div.z;
    tz[!i] = (max.z - pos.z) * div.z;

    if (div.z == INFINITY) {
        tz[0] = (min.z > pos.z) ? INFINITY : -INFINITY;
        tz[1] = (max.z > pos.z) ? INFINITY : -INFINITY;
    }

    if ((t[0] > tz[1]) || (tz[0] > t[1]))
        return INFINITY;

    //res = fmin(res, tz[0]);

    if (tz[0] > t[0])
        t[0] = tz[0];
    if (tz[1] < t[1])
        t[1] = tz[1];

    if (t[1] < 0.0)
        return INFINITY;

    return 0.0;
}
