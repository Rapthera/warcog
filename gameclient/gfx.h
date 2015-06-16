#ifndef GFX_H
#define GFX_H

#include "gl.h"
#include "shader.h"
#include "shaders.h"
#include "text.h"

enum {
    font_ui_big,
    font_ui,
};
#define num_font 2

enum {
    tex_screen = 0,
    tex_outline = 1,
    tex_fol = 2,
    tex_fog = 3,
    tex_particle = 4,
    tex_map = 5,
    tex_ui = 6,
    tex_font = 7,
    tex_mdl = 8,
};

enum {
    fb_screen,
    fb_fol,
    fb_fog
};

enum {
    vo_post = 0,
    vo_2d = 1,
    vo_particle = 2,
    vo_map = 3,
    vo_mapc = 4,
    vo_water = 5,
    vo_strip = 6,
    vo_gs = 7,
    vo_mdl = 8,

    vo_mdl_elements = 72,
};

typedef struct {
    /* shaders */
    //mshader_t modelshader;
    shader_t shader[num_shader];
    GLuint mdl_quats, mdl_trans, mdl_outline;

    /* resources */
    void *model[64];
    GLuint texture[72], vbo[72+64], vao[72];
    font_t font[num_font];

    /* view */
    uint32_t iwidth, iheight;

    /* framebuffers */
    GLuint fb[3], screendepth;

    /* */
    const char *text;
} gfx_t;

gfx_t gfx;
#define text(x) (gfx.text + (x))

void gfx_draw(gfx_t *gfx);
void gfx_size(gfx_t *gfx, int width, int height);
bool gfx_init(gfx_t *gfx, int width, int height);
void gfx_cleanup(gfx_t *gfx);

void gfx_texture(gfx_t *gfx, const void *data, int width, int height, int i);
void gfx_model(gfx_t *gfx, const void *data, int i, int len, int nindex, int nvert);
void gfx_maptexture(gfx_t *gfx, const void *data);
void gfx_particletexture(gfx_t *gfx, const void *data);
void gfx_loadparticles(gfx_t *gfx, int count);

#endif
