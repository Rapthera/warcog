#include "text.h"
#include "text/freetype.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

typedef struct {
    uint16_t height, spacewidth;
} header_t;

typedef struct {
    uint16_t advance;
    uint8_t npoint, ncontour;
} fglyph_t;

typedef struct {
    int16_t x, y;
} point_t;

#define sublen(x) if ((len -= (x)) < 0) return -1;
#define read(x) memcpy(&x, data, sizeof(x)), data += sizeof(x)

static int fixed_mul(int a, int b)
{
    if (a < 0)
        return -(((int64_t)-a * b + 0x8000) >> 16);
    else
        return (((int64_t)a * b + 0x8000) >> 16);
}

static int fixed_mul2(int a, int b)
{
    return ((((int64_t)a * b) + (1 << 21)) >> 22);
}

int loadfont(font_t *font, const int *sizes, int count, const void *data, int len)
{
    header_t h;
    fglyph_t f;
    uint8_t c;
    point_t p;
    const void *start;
    int i, j, k;

    FT_Raster r;
    uint64_t pool[768] = {0};

    int x, y, lineheight;

    short contour[16];
    char tag[256];
    FT_Vector point[256], point_trans[256];
    FT_Outline outline;
    FT_Raster_Params params;
    FT_Bitmap bitmap;
    int xmin, xmax, ymin, ymax;
    int txmin, txmax, tymin, tymax;
    int width, height, size;
    glyph_t *g;

    ft_grays_raster.raster_new(NULL, &r);
    ft_grays_raster.raster_reset(r, (void*)pool, sizeof(pool));

    outline.points = point_trans;
    outline.tags = tag;
    outline.contours = contour;
    outline.flags = 0;

    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap.num_grays  = 256;
    bitmap.width = 1024;
    bitmap.rows = 1024;
    bitmap.pitch = 1024;

    bitmap.buffer = malloc(1024 * 1024);
    if (!bitmap.buffer)
        return -1;
    memset(bitmap.buffer, 0, 1024 * 1024);

    params.source = &outline;
    params.flags = FT_RASTER_FLAG_AA;

    x = 0;
    y = 0;
    lineheight = 0;

    read(h);

    start = data;
    for (k = 0; k < count; data = start, k++) {
        font[k].height = fixed_mul2(h.height, sizes[k]);
        font[k].spacewidth = fixed_mul2(h.spacewidth, sizes[k]);

        for (i = 0; i < 94; i++) {
            read(f);

            for (j = 0; j < f.ncontour; j++) {
                read(c);
                contour[j] = c;
            }

            xmin = xmax = ymin = ymax = 0;
            for (j = 0; j < f.npoint; j++) {
                read(p);
                tag[j] = p.x & 1;
                point[j].x = p.x / 2;
                if (xmin > point[j].x)
                    xmin = point[j].x;

                if (xmax < point[j].x)
                    xmax = point[j].x;

                point[j].y = p.y;
                if (ymin > point[j].y)
                    ymin = point[j].y;

                if (ymax < point[j].y)
                    ymax = point[j].y;
            }

            outline.n_contours = f.ncontour;
            outline.n_points = f.npoint;

            size = sizes[k];
            g = &font[k].glyph[i];

            txmin = (fixed_mul(xmin, size) >> 6);
            txmax = ((fixed_mul(xmax, size) + 63) >> 6);
            tymin = (fixed_mul(ymin, size) >> 6);
            tymax = ((fixed_mul(ymax, size) + 63) >> 6);

            width = txmax - txmin;
            height = tymax - tymin;

            if (x + width >= 1024) {
                y += lineheight;
                lineheight = 0;
                x = 0;
            }

            if (lineheight < height)
                lineheight = height;

            g->x = x;
            g->y = (y + height);
            g->width = width;
            g->height = height;
            g->left = txmin;
            g->top = tymax;
            g->advance = fixed_mul2(f.advance, size);

            for (j = 0; j < f.npoint; j++) {
                point_trans[j].x = fixed_mul(point[j].x, size) + ((x - txmin) << 6);
                point_trans[j].y = fixed_mul(point[j].y, size) + ((y - tymin) << 6);
            }

            x += width;

            params.target = &bitmap;
            ft_grays_raster.raster_render(r, &params);
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024, 1024, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.buffer);
    free(bitmap.buffer);
    return 0;
}

int textwidth(const font_t *font, const char *str)
{
    int width;
    uint8_t ch;

    width = 0;
    while ((ch = *str++)) {
        if (!(ch -= ' '))
            width += font->spacewidth;
        else if (--ch < 94)
            width += font->glyph[ch].advance;
    }

    return width;
}

static int wordwidth(const font_t *font, const char *str, int max)
{
    int width;
    uint8_t ch;

    width = 0;
    while ((ch = *str++)) {
        if (!(ch -= ' '))
            break;

        if (--ch < 94) {
            width += font->glyph[ch].advance;
            if (width > max)
                return -1;
        }

    }

    return width;
}

vert2d_t* textverts(vert2d_t *v, const font_t *font, const char *str, int x, int y, uint32_t color)
{
    uint8_t ch;
    const glyph_t *g;

    while ((ch = *str++)) {
        if (!(ch -= ' ')) {
            x += font->spacewidth;
            continue;
        }

        if (--ch >= 94)
            continue;

        g = &font->glyph[ch];
        //printf("%u %u %u %u\n", g->width, g->height, g->x, g->y);
        *v++ = vert2d_text(x - g->left, y - g->top, g->width, g->height, g->x, g->y, color);
        x += g->advance;
    }

    return v;
}

int textboxheight(const font_t *font, const char *str, int w)
{
    uint8_t ch;
    const glyph_t *g;
    int y, x, width;

    y = font->height;
    x = 0;
    do {
        width = wordwidth(font, str, w);
        if (width < 0) {
            do {
                ch = *str++;
                if (!ch)
                    return y;

                if ((ch -= (' ' + 1)) >= 94)
                    continue;

                g = &font->glyph[ch];
                if (x + g->width > w) {
                    x = 0;
                    y += font->height;
                    break;
                }

                x += g->advance;
            } while(1);
        } else {
            if (width > w - x) {
                x = 0;
                y += font->height;
            }

            do {
                ch = *str++;
                if (!ch)
                    return y;

                if (!(ch -= ' ')) {
                    x += font->spacewidth;
                    break;
                }

                if (--ch >= 94)
                    continue;

                x += font->glyph[ch].advance;
            } while(1);
        }
    } while (1);
}

vert2d_t* textbox(vert2d_t *v, const font_t *font, const char *str, int px, int y, int w, uint32_t color)
{
    uint8_t ch;
    const glyph_t *g;
    int x, width;

    x = 0;
    do {
        width = wordwidth(font, str, w);
        if (width < 0) {
            do {
                ch = *str++;
                if (!ch)
                    return v;

                if ((ch -= (' ' + 1)) >= 94)
                    continue;

                g = &font->glyph[ch];
                if (x + g->width > w) {
                    x = 0;
                    y += font->height;
                    break;
                }

                *v++ = vert2d_text(px + x - g->left, y - g->top, g->width, g->height, g->x, g->y, color);
                x += g->advance;
            } while(1);
        } else {
            if (width > w - x) {
                x = 0;
                y += font->height;
            }

            do {
                ch = *str++;
                if (!ch)
                    return v;

                if (!(ch -= ' ')) {
                    x += font->spacewidth;
                    break;
                }

                if (--ch >= 94)
                    continue;

                g = &font->glyph[ch];
                *v++ = vert2d_text(px + x - g->left, y - g->top, g->width, g->height, g->x, g->y, color);
                x += g->advance;
            } while(1);
        }
    } while (1);
}
