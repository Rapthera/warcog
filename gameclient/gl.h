#ifndef GL_H
#define GL_H

#include <stdint.h>

typedef struct {
    int16_t x, y;
    uint16_t w, h;
    uint32_t tex;
    uint32_t color;
} vert2d_t;

vert2d_t vert2d(int x, int y, int w, int h, int tx, int ty, uint32_t color);
vert2d_t vert2d_icon(int x, int y, int id, int s, uint32_t color);
vert2d_t vert2d_scale(int x, int y, int w, int h, int tx, int ty, int s, uint32_t color);
vert2d_t vert2d_solid(int x, int y, int w, int h, uint32_t color);
vert2d_t vert2d_text(int x, int y, int w, int h, int tx, int ty, uint32_t color);

#define rgba(r,g,b,a) ((r) | ((g) << 8) | ((b) << 16) | ((a) << 24))

#ifdef XLIB
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

#ifdef WIN32
#include <GL/gl.h>
#include <GL/glext.h>
#include "win32/wgl.h"

#undef NEAR
#undef FAR
#undef near
#undef far
#undef min
#undef max

#endif

#endif
