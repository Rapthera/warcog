#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include <ft2build.h>
#include <freetype.h>

typedef struct {
    uint16_t height, spacewidth;
} header_t;

typedef struct {
    uint16_t advance;
    uint8_t npoint, ncontour;
} glyph_t;

typedef struct {
    bool tag : 1;
    int16_t x : 15;
    int16_t y;
} point_t;

int main(void)
{
    FILE *file;
    FT_Library library;
    FT_Face face;
    FT_GlyphSlot slot;
    FT_Outline *outline;
    FT_Vector kerning, *point;
    int i, j, nkern;
    int maxadvance, maxpoints, maxcontours, minkern, maxkern, maxnkern, maxcontour;
    int minx, maxx, miny, maxy;
    //point_t points[65536], *pp = points;
   // uint8_t contour[2048], *cp = contour;

    file = fopen("font", "wb");
    if (!file)
        return 0;

    if (FT_Init_FreeType(&library))
        return 0;

    if (FT_New_Face(library, "font.ttf", 0, &face))
        return 0;

    /* width of space */
    if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ' '), FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING))
        return 0;

    header_t h = {face->height, (int)face->glyph->advance.x};
    fwrite(&h, sizeof(h), 1, file);

    printf("height, spacewidth = %i %i\n", face->height, (int)face->glyph->advance.x);

    printf("test %i %i\n", face->ascender, face->size->metrics.ascender);
    int ascender = 736;

    maxadvance = 0; maxpoints = 0; maxcontours = 0;
    minkern = 0; maxkern = 0; maxnkern = 0;
    maxcontour = 0;
    minx = maxx = miny = maxy = 0;
    for (i = ' ' + 1; i <= 126; i++) {
        if (FT_Load_Glyph(face, FT_Get_Char_Index(face, i), FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING))
            return 0;

        slot = face->glyph;
        outline = &slot->outline;

        nkern = 0;
        for (j = ' '; j <= 126; j++) {
            FT_Get_Kerning(face, FT_Get_Char_Index(face, j), FT_Get_Char_Index(face, i),
                           FT_KERNING_UNSCALED, &kerning);

            if (kerning.x)
                nkern++;

            if (maxkern < kerning.x)
                maxkern = kerning.x;

            if (minkern > kerning.x)
                minkern = kerning.x;

            if (kerning.y)
                printf("warning: ykern %i %i %i\n", j, i, (int)kerning.y);
        }

        if (maxadvance < slot->advance.x)
            maxadvance = slot->advance.x;

        if (maxpoints < outline->n_points)
            maxpoints = outline->n_points;

        if (maxcontours < outline->n_contours)
            maxcontours = outline->n_contours;

        if (maxnkern < nkern)
            maxnkern = nkern;

        if (outline->flags)
            printf("warning: outline flags %u\n", outline->flags);

        glyph_t g = {slot->advance.x, outline->n_points, outline->n_contours};
        fwrite(&g, sizeof(g), 1, file);

        for (j = 0; j < outline->n_contours; j++) {
            if (maxcontour < outline->contours[j])
                maxcontour = outline->contours[j];

            uint8_t contour = outline->contours[j];
            fwrite(&contour, 1, 1, file);
            //*cp++ = outline->contours[j];
        }

        for (j = 0; j < outline->n_points; j++) {
            point = &outline->points[j];

            if (maxx < point->x)
                maxx = point->x;

            if (maxy < point->y)
                maxy = point->y;

            if (minx > point->x)
                minx = point->x;

            if (miny > point->y)
                miny = point->y;

            if (outline->tags[j] != 0 && outline->tags[j] != 1)
                printf("warning: unsupport outline tag: %u %u\n", j, outline->tags[j]);

            point_t p = {outline->tags[j], point->x, point->y - ascender};
            fwrite(&p, sizeof(p), 1, file);
            //*pp++ = p;
        }

        //fwrite(outline->tags, outline->n_points, 1, file);
    }

   // assert((void*)pp <= (void*)points + sizeof(points));
    //assert((void*)cp <= (void*)contour + sizeof(contour));

   // fwrite(points, sizeof(*pp), pp - points, file);
    //fwrite(contour, sizeof(*cp), cp - contour, file);


    printf("%u %u (%u %u) (%u %i %i) (%i %i %i %i) %u\n", maxadvance, maxpoints, maxcontours, maxcontour,
           maxnkern, minkern, maxkern, minx, maxx, miny, maxy, 126 - ' ');
    return 0;
}
