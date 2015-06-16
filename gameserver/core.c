#include "core.h"
#include <math.h>



point makepoint(uint32_t x, uint32_t y)
{
    point res;

    res.x = x;
    res.y = y;
    return res;
}

point addvec(point p, vec v)
{
    return point(p.x + (int32_t) round(v.x), p.y + (int32_t) round(v.y));
}

uint64_t dist2(point a, point b)
{
    int32_t dx, dy;

    dx = b.x - a.x;
    dy = b.y - a.y;

    return ((int64_t)dx * dx) + ((int64_t)dy * dy);
}

bool inrange(point a, point b, uint32_t range)
{
    return (dist2(a, b) <= (uint64_t)range * range);
}

bool inrange_line(point *res, point a, point b, vec dir, uint32_t range)
{
    vec v, n;
    double d;

    v = vec_p(a, b);
    d = dot(v, dir);
    if (d < 0.0)
        return 0;

    n = vec(dir.y, -dir.x);
    if (fabs(dot(v, n)) > range)
        return 0;

    *res = addvec(a, scale(dir, d));
    return 1;
}

uint16_t getangle(point a, point b)
{
    int32_t dx, dy;

    dx = b.x - a.x;
    dy = b.y - a.y;

    return (int)round(atan2(dy, dx) * full_circle / (2.0 * M_PI));
}

delta makedelta(point a, point b)
{
    delta d;

    d.x = b.x - a.x;
    d.y = b.y - a.y;

    return d;
}

vec makevec(double x, double y)
{
    vec res;

    res.x = x;
    res.y = y;
    return res;
}

vec vec_angle(double angle, double distance)
{
    angle = angle * M_PI * 2.0 / full_circle;
    return vec(cos(angle) * distance, sin(angle) * distance);
}

vec vec_p(point a, point b)
{
    int32_t dx, dy;

    dx = b.x - a.x;
    dy = b.y - a.y;

    return vec(dx, dy);
}

vec vec_normp(point a, point b)
{
    vec v = vec_p(a, b);
    return scale(v, 1.0 / mag(v));
}

vec scale(vec v, double k)
{
    return vec(v.x * k, v.y * k);
}

vec add(vec a, vec b)
{
    return vec(a.x + b.x, a.y + b.y);
}

double dot(vec a, vec b)
{
    return a.x * b.x + a.y * b.y;
}

double mag(vec v)
{
    return sqrt(dot(v, v));
}

owner_t makeowner(uint8_t team, uint8_t id)
{
    owner_t res;

    res.team = team;
    res.id = id;
    return res;
}

anim_t makeanim(uint8_t id, uint8_t len)
{
    anim_t res;

    res.frame = 0;
    res.id = id;
    res.len = len;
    return res;
}
