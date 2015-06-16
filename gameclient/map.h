#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include "gl.h"

typedef struct {
    uint8_t height[4];
} tile_t;

typedef struct {
    uint32_t info;
    uint8_t height[4];
} mapvert_t;

typedef struct {
    uint32_t data[2];
} watervert_t;

typedef struct {
    uint8_t min_h, max_h;
} maptree_t;

typedef struct {
    int x, y, i, depth;
} tree_t;

typedef struct {
    uint8_t type, value;
    uint16_t x, y, x2, y2;
} deform_t;

typedef struct {
    int size_shift, size, nvert, size3;
    maptree_t *tree;
    mapvert_t *vert;
    int *tmap;
    deform_t deform[255];
} map_t;

map_t map;

float map_height(const map_t *map, uint32_t ix, uint32_t iy);
vec3 map_intersect_tile(const map_t *map, vec3 pos, vec3 ray, int x, int y);

bool map_load(map_t *map, const void *data, int size_shift);
void map_free(map_t *map);

void map_deform(map_t *map, uint8_t id, const uint8_t *data);
void map_undeform(map_t *map, uint8_t id);
void map_reset(map_t *map);

#endif
