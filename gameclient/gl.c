#include "gl.h"

vert2d_t vert2d(int x, int y, int w, int h, int tx, int ty, uint32_t color)
{
    vert2d_t v = {
        x, y, w, h, (tx | (ty << 12) | (8 << 24) | (1 << 30)), color
    };
    return v;
}

vert2d_t vert2d_icon(int x, int y, int id, int s, uint32_t color)
{
    int tx, ty;

    tx = (id & 15) * 64;
    ty = (id >> 4) * 64;

    vert2d_t v = {
        x, y, 64, 64, (tx | (ty << 12) | (s << 25) | (3 << 30)), color
    };
    return v;
}

vert2d_t vert2d_scale(int x, int y, int w, int h, int tx, int ty, int s, uint32_t color)
{
    vert2d_t v = {
        x, y, w, h, (tx | (ty << 12) | (s << 24) | (1 << 30)), color
    };
    return v;
}

vert2d_t vert2d_solid(int x, int y, int w, int h, uint32_t color)
{
    vert2d_t v = {
        x, y, w, h, (8 << 24), color
    };
    return v;
}

vert2d_t vert2d_text(int x, int y, int w, int h, int tx, int ty, uint32_t color)
{
    vert2d_t v = {
        x, y, w, h, (tx | ((1024 - ty) << 12) | (8 << 24) | (2 << 30)), color
    };
    return v;
}
