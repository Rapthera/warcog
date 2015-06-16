#include "map.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "gl.h"
#include "gfx.h"

#define MAX_SIZE 11

static void tree_xy(int *r_x, int *r_y, int i)
{
    int x, y, j;

    x = 0; y = 0;
    for (j = 23; j >= 0;) {
        y = (y | (i & (1 << j--))) >> 1;
        x = ((x >> 1) | (i & (1 << j--)));
    }

    *r_x = x;
    *r_y = y;
}

/*static int tree_i(int x, int y)
{
    int i, j;

    i = 0;
    for (j = 0; j < 12; j++) {
        i |= ((x & (1 << j)) << j);
        i |= ((y & (1 << j)) << (j + 1));
    }

    return i;
}*/

static uint8_t max(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}

static uint8_t min(uint8_t a, uint8_t b)
{
    return (a < b) ? a : b;
}

float map_height(const map_t *map, uint32_t ix, uint32_t iy)
{
    const mapvert_t *t;
    int mx, my;
    float x, y, res;

    mx = ix / (65536 * 16);
    my = iy / (65536 * 16);

    if (mx >= map->size || my >= map->size)
        return 0.0;

    t = &map->vert[map->tmap[(my << map->size_shift) + mx]];
    x = 1.0 - (float) (ix % (65536 * 16)) / (65536.0 * 16.0);
    y = (float) (iy % (65536 * 16)) / (65536.0 * 16.0);

    res = t->height[1];
    if (y < x) {
        res += x * ((int)t->height[0] - t->height[1]) + y * ((int)t->height[2] - t->height[0]);
    } else {
        res += x * ((int)t->height[2] - t->height[3]) + y * ((int)t->height[3] - t->height[1]);
    }

    return res;
}

vec3 map_intersect_tile(const map_t *map, vec3 pos, vec3 ray, int x, int y)
{
    const mapvert_t *t;
    vec3 u, v, p, n;
    float d, xr, yr;

    t = &map->vert[map->tmap[(y << map->size_shift) + x]];
    p = sub3(vec3((x + 1) * 16, y * 16, t->height[1]), pos);

    u = vec3(0, 16, (int)t->height[2] - t->height[0]);
    v = vec3(16, 0, (int)t->height[1] - t->height[0]);

    n = cross3(u, v);
    d = dot3(p, n) / dot3(ray, n);
    xr = d * ray.x - p.x + 16.0;
    yr = d * ray.y - p.y;

    if (xr >= 0.0 && yr >= 0.0 && yr + xr <= 16.0)
        return vec3(x * 16 + xr, y * 16 + yr, d);

    u = vec3(-16, 0, (int)t->height[2] - t->height[3]);
    v = vec3(0, -16, (int)t->height[1] - t->height[3]);

    n = cross3(u, v);
    d = dot3(p, n) / dot3(ray, n);
    xr = d * ray.x - p.x + 16.0;
    yr = d * ray.y - p.y;

    if (xr <= 16.0 && yr <= 16.0 && yr + xr >= 16.0)
        return vec3(x * 16 + xr, y * 16 + yr, d);

    return vec3(0.0, 0.0, INFINITY);
}

bool map_load(map_t *map, const void *data, int size_shift)
{
    const tile_t *tile, *t;
    maptree_t *tree, *root, *base, *tr_parent, *tr;
    mapvert_t *vert, *v;
    int *tmap;
    int size, i, j, k, id;
    int x, y;
    const uint8_t *tile_id;

    if (size_shift > MAX_SIZE)
        return 0;

    size = 1 << size_shift;
    k = size * size;

    tile_id = data; data += k;
    tile = data; data += k * sizeof(tile_t);
    data += k;//pathing

    tmap = malloc(k * 4);
    if (!tmap)
        return 0;

    j = k / 3;
    tree = malloc((k + j) * sizeof(maptree_t));
    if (!tree)
        goto EXIT_FREE_TMAP;

    vert = malloc(k * sizeof(mapvert_t));
    if (!vert)
        goto EXIT_FREE_TREE;

    v = vert;
    root = tree + j;
    for (i = 0; i < k; i++) {
        tr = &root[i];

        tree_xy(&x, &y, i);
        tmap[(y << size_shift) + x] = i;

        id = tile_id[(y << size_shift) + x];
        t = &tile[(y << size_shift) + x];

        v->info = x | (y << 12) | (id << 24);
        memcpy(v->height, t->height, 4);
        v++;

        tr->min_h = min(min(t->height[0], t->height[1]), min(t->height[2], t->height[3]));
        tr->max_h = max(max(t->height[0], t->height[1]), max(t->height[2], t->height[3]));
    }

    k = size;
    while (k >>= 1) {
        base = root;
        root -= k * k;

        for (i = 0; i < k * k; i++) {
            tr_parent = &root[i];
            tr = &base[i * 4];

            tr_parent->min_h = min(min(tr[0].min_h, tr[1].min_h), min(tr[2].min_h, tr[3].min_h));
            tr_parent->max_h = max(max(tr[0].max_h, tr[1].max_h), max(tr[2].max_h, tr[3].max_h));
        }
    }

    k = v - vert;
    v = realloc(vert, k * sizeof(mapvert_t));
    if (!v)
        goto EXIT_FREE_VERT;

    glBindBuffer(GL_ARRAY_BUFFER, gfx.vbo[vo_map]);
    glBufferData(GL_ARRAY_BUFFER, k * sizeof(mapvert_t), v, GL_STATIC_DRAW);
    glBindVertexArray(gfx.vao[vo_map]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 8, 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 8, (void*)4);

    map->size_shift = size_shift;
    map->size = size;
    map->size3 = size * size / 3;
    map->nvert = k;
    map->tmap = tmap;
    map->tree = tree;
    map->vert = v;
    return 1;

EXIT_FREE_VERT:
    free(vert);
EXIT_FREE_TREE:
    free(tree);
EXIT_FREE_TMAP:
    free(tmap);
    return 0;
}

void map_free(map_t *map)
{
    free(map->tmap);
    free(map->tree);
    free(map->vert);
}

/* doesnt work when cliffs are involved */
static void deform_tile(map_t *map, int x, int y, int change, int flags)
{
    int minh, maxh, i, j;
    mapvert_t *t;
    maptree_t *tr;

    i = map->tmap[(y << map->size_shift) + x];
    tr = &map->tree[j = (map->size3 + i)];
    t = &map->vert[i];

    if (flags == 15) {
        t->height[0] += change;
        t->height[1] += change;
        t->height[2] += change;
        t->height[3] += change;

        maxh = tr->max_h + change;
        minh = tr->min_h + change;
    } else {
        if (flags & 1)
            t->height[0] += change;

        if (flags & 2)
            t->height[1] += change;

        if (flags & 4)
            t->height[2] += change;

        if (flags & 8)
            t->height[3] += change;

        maxh = max(max(t->height[0], t->height[1]), max(t->height[2], t->height[3]));
        minh = min(min(t->height[0], t->height[1]), min(t->height[2], t->height[3]));
    }

    tr->max_h = maxh;
    tr->min_h = minh;
    while (tr = &map->tree[j = ((j - 1) >> 2)], tr->max_h < maxh || tr->min_h > minh) {
        if (tr->max_h < maxh)
            tr->max_h = maxh;
        if (tr->min_h > minh)
            tr->min_h = minh;

        if (!j)
            break;
    }
}

static void deform(map_t *map, int xmin, int ymin, int xmax, int ymax, int change)
{
    int x, y;

    for (y = ymin; y < ymax; y++) {
        for (x = xmin; x < xmax; x++) {
            deform_tile(map, x, y, change, 15);
        }
    }

    if (ymin) {
        y = ymin - 1;
        for (x = xmin; x < xmax; x++)
            deform_tile(map, x, y, change, 12);

        if (xmin)
            deform_tile(map, xmin - 1, y, change, 8);

        if (xmax != map->size)
            deform_tile(map, xmax, y, change, 4);
    }

    if (ymax != map->size) {
        y = ymax;
        for (x = xmin; x < xmax; x++)
            deform_tile(map, x, y, change, 3);

        if (xmin)
            deform_tile(map, xmin - 1, y, change, 2);

        if (xmax != map->size)
            deform_tile(map, xmax, y, change, 1);
    }

    if (xmin) {
        x = xmin - 1;
        for (y = ymin; y < ymax; y++)
            deform_tile(map, x, y, change, 10);
    }

    if (xmax != map->size) {
        x = xmax;
        for (y = ymin; y < ymax; y++)
            deform_tile(map, x, y, change, 5);
    }

    /* TODO: only update necessary */
    glBindBuffer(GL_ARRAY_BUFFER, gfx.vbo[vo_map]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, map->nvert * sizeof(mapvert_t), map->vert);
}

void map_deform(map_t *map, uint8_t id, const uint8_t *data)
{
    deform_t *d;

    d = &map->deform[id];
    map_undeform(map, id);

    d->type = *data++;
    d->value = *data++;
    memcpy(&d->x, data, 8);

    if (d->type == 0)
        deform(map, d->x, d->y, d->x2, d->y2, (int8_t)d->value);
}

void map_undeform(map_t *map, uint8_t id)
{
    deform_t *d;

    d = &map->deform[id];
    if (d->type == 0 && d->value) {
        deform(map, d->x, d->y, d->x2, d->y2, -(int8_t)d->value);
        d->value = 0;
    }
    d->type = 0;
    d->value = 0;
}

void map_reset(map_t *map)
{
    int i;

    for (i = 0; i < 255; i++)
        map_undeform(map, i);
}

