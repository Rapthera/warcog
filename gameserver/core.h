#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define distance(x) ((x) * 65536)
#define distance_max (~0)
#define speed(x) ((distance(x) + FPS/2) / FPS)
#define accel(x) ((speed(x) + FPS/2) / FPS)
#define turnrate(x) (((x) + FPS/2) / FPS)

#define hp(x) ((x) * 65536)
#define hpsec(x) ((hp(x) + FPS/2) / FPS)

#define mana(x) ((x) * 65536)
#define manasec(x) ((mana(x) + FPS/2) / FPS)

#define msec(x) (((x) * FPS + 500) / 1000)

#define tile(x) distance((x) * 16)
#define tiles(x,y) position((x) * 16, (y) * 16)

#define full_circle 65536
#define half_circle (full_circle / 2)

typedef struct {
    uint32_t x, y;
} point;

typedef struct {
    int32_t x, y;
} delta;

typedef struct {
    double x, y;
} vec;

typedef uint16_t angle_t;

point makepoint(uint32_t x, uint32_t y);
#define point(x, y) makepoint(x, y)
#define position(x, y) point(distance(x), distance(y))
point addvec(point p, vec v);
uint64_t dist2(point a, point b);
bool inrange(point a, point b, uint32_t range);
bool inrange_line(point *res, point a, point b, vec dir, uint32_t range);
uint16_t getangle(point a, point b);

delta makedelta(point a, point b);
#define delta(a, b) makedelta(a, b)

vec makevec(double x, double y);
#define vec(x, y) makevec(x, y)
vec vec_angle(double angle, double distance);
vec vec_p(point a, point b);
vec vec_normp(point a, point b);
vec scale(vec v, double k);
vec add(vec a, vec b);
double dot(vec a, vec b);
double mag(vec v);

typedef struct {
    uint8_t id, target_type;
    uint16_t timer;
    union {
        uint16_t ent_id;
        point p;
    };
} order_t;

typedef union {
    uint16_t ent;
    point pos;
} target_t;

typedef struct {
    uint8_t team, id;
} owner_t;

owner_t makeowner(uint8_t team, uint8_t id);
#define owner(x, y) makeowner(x, y)
#define owner_none owner(0xFF, 0xFF)

typedef struct {
    uint8_t team, id;
    uint16_t hero;
    uint8_t color[3];
    uint8_t empty[9];
    uint8_t name[16];
} player_t;
#define playerowner(p) (owner(p->team, p->id))

typedef struct {
    uint16_t frame;
    uint8_t id, len;
} anim_t;

anim_t makeanim(uint8_t id, uint8_t len);
#define anim(x, y) makeanim(x, y)



