#ifndef TEXT_H
#define TEXT_H

#include "gl.h"

typedef struct {
    uint16_t x;
    uint8_t y, advance;
    uint8_t width, height;
    int8_t left, top;
} glyph_t;

typedef struct {
    uint8_t y, height, spacewidth;
    glyph_t glyph[94];
} font_t;

int loadfont(font_t *font, const int *sizes, int count, const void *data, int len);

int textwidth(const font_t *font, const char *str);
int textboxheight(const font_t *font, const char *str, int w);

vert2d_t* textverts(vert2d_t *v, const font_t *font, const char *str, int x, int y, uint32_t color);
vert2d_t* textbox(vert2d_t *v, const font_t *font, const char *str, int px, int y, int w, uint32_t color);

#endif
