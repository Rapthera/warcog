#include "gfx.h"

#define countof(x) (sizeof(x)/sizeof(*(x)))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "view.h"
#include "model.h"
#include "map.h"
#include "entity.h"
#include "input.h"
#include "util.h"
#include "ui.h"
#include "net.h"
#include "player.h"
#include "particle.h"

typedef struct {
    mapvert_t m;
    float ox, oy;
} mapcvert_t;

static void draw_map(gfx_t *gfx, map_t *map)
{
    const shader_t *s;
    tree_t tree[map->size_shift * 3 + 1], *t;
    maptree_t *mt;
    vec3 min, max;
    int res, size;

    s = &gfx->shader[s_map];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &view.matrix.m[0][0]);

    glBindVertexArray(gfx->vao[vo_map]);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_map]);
    /*
    glBindTexture(GL_TEXTURE_2D, gfx->maptexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gfx->fogtex);
    glActiveTexture(GL_TEXTURE0)
    */

    //int ndraw = 0, ndrawi = 0;

    t = tree;
    t->x = 0;
    t->y = 0;
    t->i = 0;
    t->depth = 0;

    do {
        mt = &map->tree[((1 << (t->depth * 2)) / 3) + t->i];
        size = (1 << (map->size_shift - t->depth));

        min = vec3(t->x * 16, t->y * 16, mt->min_h);
        max = vec3((t->x + size) * 16, (t->y + size) * 16, mt->max_h);

        res = view_intersect_box(&view, min, max, vec2(-1.0, 1.0), vec2(-1.0, 1.0));
        if (res < 0) {
            t--;
        } else if (res || t->depth == map->size_shift) {
            glDrawArrays(GL_POINTS, size * size * t->i, size * size);
            //ndraw += size * size;
            //ndrawi++;
            t--;
        } else {
            size /= 2;
            t->i *= 4;
            t->depth++;

            t[1].x = t->x + size;
            t[1].y = t->y;
            t[1].i = t->i + 1;
            t[1].depth = t->depth;

            t[2].x = t->x;
            t[2].y = t->y + size;
            t[2].i = t->i + 2;
            t[2].depth = t->depth;

            t[3].x = t->x + size;
            t[3].y = t->y + size;
            t[3].i = t->i + 3;
            t[3].depth = t->depth;

            t += 3;
        }
    } while (t >= tree);

    //printf("drawn %u %u\n", ndraw, ndrawi);
}

static mapcvert_t* mapcircle(map_t *map, mapcvert_t *v, mapcvert_t *max, uint32_t px, uint32_t py,
                             float r)
{
    int minx, miny, maxx, maxy, x, y, radius;
    vec2 o;

    radius = r > 255.0 ? 255 : (r + 0.5);
    o = vec2(px / 65536.0, py / 65536.0);

    minx = (int) (o.x - r) / 16;
    maxx = (int) (o.x + r) / 16;

    miny = (int) (o.y - r) / 16;
    maxy = (int) (o.y + r) / 16;

    if (minx < 0)
        minx = 0;

    if (miny < 0)
        miny = 0;

    if (maxx >= map->size)
        maxx = map->size - 1;

    if (maxy >= map->size)
        maxy = map->size - 1;

    for (y = miny; y <= maxy; y++) {
        for (x = minx; x <= maxx && v != max; x++) {
            v->m = map->vert[map->tmap[(y << map->size_shift) + x]];
            v->m.info = (v->m.info & 0xFFFFFF) | (radius << 24);
            v->ox = (x * 16) - o.x;
            v->oy = (y * 16) - o.y;
            v++;
        }
    }

    return v;
}

static void draw_selection(gfx_t *gfx, input_t *in, ents_t *ents, map_t *map)
{
    entity_t *ent;
    const shader_t *s;
    const entitydef_t *def;
    int i;
    mapcvert_t vert[4096], *v;

    v = vert;

    /* */
    if (in->pradius > 0)
        v = mapcircle(map, v, &vert[4096], in->px, in->py, in->pradius);

    /* selection circles */
    for (i = 0; i < in->sel.count; i++) {
        ent = &ents->ent[in->sel.ent[i]];
        def = &entitydef[ent->def];

        v = mapcircle(map, v, &vert[4096], ent->x, ent->y, def->boundsradius);
    }

    i = v - vert;
    s = &gfx->shader[s_mapc];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &view.matrix.m[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_mapc]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(*v), vert);

    glBindVertexArray(gfx->vao[vo_mapc]);
    glDrawArrays(GL_POINTS, 0, i);

}

static void _getvalues(const entity_t *ent, const effect_t *effect, uint16_t id,
                       vec4 *color, uint16_t *tex)
{
    const effectdef_t *def;
    const mdleffectdef_t *mdef;
    int i;
    effectid_t d;

    def = &effectdef[id];
    for (i = 0; i < def->count; i++) {
        d = def->id[i];
        switch (d.type) {
        case effect_mdl:
            mdef = &mdleffectdef[d.i];
            *color = vec4(mdef->color[0] / 255.0, mdef->color[1] / 255.0,
                          mdef->color[2] / 255.0, mdef->color[3] / 255.0);
            *tex = mdef->texture;
            break;
        }
    }
}

static void getvalues(const entity_t *ent, vec4 *color, uint16_t *tex)
{
    const state_t *s;
    const entitydef_t *def;
    const statedef_t *sdef;
    int i;

    *color = vec4(1.0, 1.0, 1.0, 1.0);
    *tex = 0xFFFF;

    def = &entitydef[ent->def];
    if (def->effect != 0xFFFF)
        _getvalues(ent, &ent->effect, def->effect, color, tex);

    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];
        sdef = &statedef[s->def];
        if (sdef->effect != 0xFFFF)
            _getvalues(ent, &s->effect, sdef->effect, color, tex);
    }
}

static void draw_ents(gfx_t *gfx, ents_t *ents)
{
    const GLenum bufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    const entity_t *ent;
    const entitydef_t *def;

    const player_t *p;
    const shader_t *s;
    int i, id;

    float theta;
    vec3 min, max;
    matrix4_t m;
    bool outline;
    vec3 outline_color;
    vec4 color, team_color;
    uint16_t texture;

    s = &gfx->shader[s_model];
    glUseProgram(s->prog);

    p = &players.player[self_id()];

    for (i = 0; i < ents->count; i++) {
        ent = &ents->ent[id = ents->list[i]];
        def = &entitydef[ent->def];

        if (def->model == 0xFFFF)
            continue;

        if (id == input.target_ent) {
            outline = 1;
            outline_color = ((ent->team == 0xFF && ent->owner != self_id()) || ent->team != p->team)
                ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
        } else {
            outline = 0;
        }

        theta = (float) ent->angle * (2.0 * M_PI / 65536.0);

        min = sub3(ent->pos, vec3(def->boundsradius, def->boundsradius, 0.0));
        max = add3(ent->pos, vec3(def->boundsradius, def->boundsradius, def->boundsheight));

        if (view_intersect_box(&view, min, max, vec2(-1.0, 1.0), vec2(-1.0, 1.0)) < 0)
            continue;

        m = view_object(&view, ent->pos, theta, def->modelscale);
        glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &m.m[0][0]);
        glBindVertexArray(gfx->vao[vo_mdl + def->model]);

        if (outline) {
            glDrawBuffers(2, bufs);
            glUniform3fv(gfx->mdl_outline, 1, &outline_color.x);
        }

        team_color = vec4(0.0, 0.0, 0.0, 1.0);
        getvalues(ent, &color, &texture);

        model_draw(s, gfx->mdl_trans, gfx->mdl_quats, &gfx->texture[tex_mdl], gfx->model[def->model],
                   def->anim[ent->anim], (double) ent->anim_frame / ent->anim_len, &team_color.x,
                   &color.x, texture);

        if (outline)
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }
}

typedef struct {
    vec3 start, end;
    uint32_t data[2];
} stripvert_t;

typedef struct {
    vec3 pos;
    uint32_t data;
} gsvert_t;

static stripvert_t* effect_strips(stripvert_t *v, const entity_t *ent, const effect_t *effect,
                                  uint16_t id)
{
    const entity_t *proxy;
    const effectdef_t *def;
    const stripdef_t *sdef;
    int i;
    effectid_t d;

    def = &effectdef[id];
    proxy = ents_get(&ents, ent->proxy);
    for (i = 0; i < def->count; i++) {
        d = def->id[i];
        switch (d.type) {
        case effect_strip:
            sdef = &stripdef[d.i];
            if (effect->var[i] && proxy->def != 0xFFFF) {
                v->start = entity_bonepos(ent, sdef->bone1);
                v->end = entity_bonepos(proxy, sdef->bone2);
                v->data[0] = ((int) (sdef->size * 16.0 + 0.5) << 16) |
                            ((int) roundf(sdef->speed * 32768.0));
                v->data[1] = effect->var[i];
                v++;
            }
            break;
        }
    }

    return v;
}

static gsvert_t* effect_gs(gsvert_t *v, const entity_t *ent, const effect_t *effect,
                                  uint16_t id)
{
    const effectdef_t *def;
    const groundspritedef_t *gdef;
    int i;
    effectid_t d;
    float s;

    def = &effectdef[id];
    for (i = 0; i < def->count; i++) {
        d = def->id[i];
        switch (d.type) {
        case effect_groundsprite:
            gdef = &groundspritedef[d.i];
            if (effect->var[i]) {
                v->pos = ent->pos;
                s = (float) (effect->var[i] - 1) / gdef->lifetime;
                v->pos.z += s * gdef->start_height + (1.0 - s) * gdef->end_height + 0.05;
                v->data = (0 << 16) | (gdef->size << 8) | gdef->texture;
                v++;
            }
            break;
        }
    }

    return v;
}

static void draw_strips(gfx_t *gfx, ents_t *ents)
{
    const entity_t *ent;
    const state_t *state;
    const entitydef_t *def;
    const statedef_t *sdef;

    const shader_t *s;
    stripvert_t vert[256], *v;
    int i, j;

    s = &gfx->shader[s_strip];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &view.matrix.m[0][0]);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_map]);

    v = vert;
    for (i = 0; i < ents->count; i++) {
        ent = &ents->ent[ents->list[i]];
        def = &entitydef[ent->def];

        for (j = 0; j < ent->nstate; j++) {
            state = &ent->state[j];
            sdef = &statedef[state->def];
            if (sdef->effect != 0xFFFF)
                v = effect_strips(v, ent, &state->effect, sdef->effect);
        }

        if (def->effect != 0xFFFF)
            v = effect_strips(v, ent, &ent->effect, def->effect);
    }
    i = v - vert;

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_strip]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(stripvert_t), vert);

    glBindVertexArray(gfx->vao[vo_strip]);
    glDrawArrays(GL_POINTS, 0, i);
}

static void draw_groundsprites(gfx_t *gfx, ents_t *ents, map_t *map)
{
    const entity_t *ent;
    const state_t *state;
    const entitydef_t *def;
    const statedef_t *sdef;

    const shader_t *s;
    gsvert_t vert[256], *v;
    int i, j;

    s = &gfx->shader[s_gs];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &view.matrix.m[0][0]);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_map]);

    v = vert;
    for (i = 0; i < ents->count; i++) {
        ent = &ents->ent[ents->list[i]];
        def = &entitydef[ent->def];

        for (j = 0; j < ent->nstate; j++) {
            state = &ent->state[j];
            sdef = &statedef[state->def];
            if (sdef->effect != 0xFFFF)
                v = effect_gs(v, ent, &state->effect, sdef->effect);
        }

        if (def->effect != 0xFFFF)
            v = effect_gs(v, ent, &ent->effect, def->effect);
    }
    i = v - vert;

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_gs]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(gsvert_t), vert);

    glBindVertexArray(gfx->vao[vo_gs]);
    glDrawArrays(GL_POINTS, 0, i);
}

static void draw_ui(gfx_t *gfx)
{
    const shader_t *s;
    vert2d_t vert[4096], *v;
    int i;

    s = &gfx->shader[s_2d];
    glUseProgram(s->prog);
    glUniform2fv(s->matrix, 1, view.matrix2d);

    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_ui]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_font]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_map]);
    glActiveTexture(GL_TEXTURE0);

    v = ui_verts(vert, &ents, input.mx, input.my);
    i = v - vert;
    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_2d]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(*v), vert);

    glBindVertexArray(gfx->vao[vo_2d]);
    glDrawArrays(GL_POINTS, 0, i);
}

static void draw_particles(gfx_t *gfx, particles_t *parts)
{
    const shader_t *s;

    s = &gfx->shader[s_particle];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &view.matrix.m[0][0]);
    glUniform1fv(s->k, 1, &view.aspect);

    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_particle]);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_particle]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particles.count * sizeof(particle_t), particles.p);

    glBindVertexArray(gfx->vao[vo_particle]);
    glDrawArrays(GL_POINTS, 0, particles.count);
}

static void draw_water(gfx_t *gfx, map_t *map)
{
    int i;
    const shader_t *s;
    const deform_t *d;
    watervert_t vert[64], *v;

    s = &gfx->shader[s_water];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, &view.matrix.m[0][0]);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_map]);

    v = vert;
    for (i = 0; i < 255; i++) {
        d = &map->deform[i];
        if (d->type != 128)
            continue;

        v->data[0] = d->x | (d->y << 12) | (4 << 24);
        v->data[1] = d->x2 | (d->y2 << 12) | (d->value << 24);
        v++;
    }
    i = v - vert;

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_water]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, i * sizeof(watervert_t), vert);

    glBindVertexArray(gfx->vao[vo_water]);
    glDrawArrays(GL_POINTS, 0, i);
}

void gfx_draw(gfx_t *gfx)
{
    shader_t *s;

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0);

    glViewport(0, 0, gfx->iwidth, gfx->iheight);

    glBindFramebuffer(GL_FRAMEBUFFER, gfx->fb[fb_screen]);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    if (net.loaded) {
        draw_map(gfx, &map);
        draw_selection(gfx, &input, &ents, &map);
        draw_ents(gfx, &ents);
        glDepthMask(GL_FALSE);
        draw_groundsprites(gfx, &ents, &map);
        draw_strips(gfx, &ents);
        draw_particles(gfx, &particles);
        glDepthMask(GL_TRUE);
        draw_water(gfx, &map);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_outline]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_screen]);

    s = &gfx->shader[s_post];
    glUseProgram(s->prog);
    glUniform2fv(s->matrix, 1, view.matrix2d);//
    glBindVertexArray(gfx->vao[vo_post]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_BLEND);

    draw_ui(gfx);

    assert(glGetError() == 0);
}

static void fbtexture(GLuint id, int width, int height, int format)
{
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, NULL);
}

void gfx_size(gfx_t *gfx, int width, int height)
{
    gfx->iwidth = width;
    gfx->iheight = height;

    int sizes[] = {0x20000, 0x10000};
    void *data = file_raw("font");
    if (data) {
        glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_font]);
        loadfont(gfx->font, sizes, 2, data, 0);
        free(data);
    } else {
        printf("failed to load fonts\n");
    }

    fbtexture(gfx->texture[tex_screen], width, height, GL_RGBA);
    fbtexture(gfx->texture[tex_outline], width, height, GL_RGBA);

    glBindRenderbuffer(GL_RENDERBUFFER, gfx->screendepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
}

bool gfx_init(gfx_t *gfx, int width, int height)
{
    int i;
    GLint val;
    const shader_t *s;
    //const GLint swizzle_white[] = {GL_ONE, GL_ONE, GL_ONE, GL_ONE};
    const GLint swizzle_text[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    const int8_t vertices[] = {-1, -1, 1, -1, -1, 1, 1, 1};

    /* check limits */
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &val);
    if (val < 2)
        return 0;

    printf("maxbuffers: %u\n", val);

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
    if (val < 8192)
        return 0;

    printf("maxtexturesize: %u\n", val);

    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    //glEnable(GL_CULL_FACE);

    /* texture 0 = white */
    glBindTexture(GL_TEXTURE_2D, 0);
    uint32_t white = ~0;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

    /* shaders */
    for (i = 0; i < num_shader; i++)
        if (!shader_load(&gfx->shader[i], shader[i].vert, shader[i].geom, shader[i].frag))
            goto cleanup;

    s = &gfx->shader[s_post];
    glUseProgram(s->prog);
    glUniform1i(s->k, 1);

    s = &gfx->shader[s_2d];
    glUseProgram(s->prog);
    glUniform1i(s->samp, 1);
    glUniform1i(s->k, 2);

    s = &gfx->shader[s_model];
    gfx->mdl_quats = glGetUniformLocation(s->prog, "r");
    gfx->mdl_trans = glGetUniformLocation(s->prog, "d");
    gfx->mdl_outline = glGetUniformLocation(s->prog, "o");


    glGenBuffers(countof(gfx->vbo), gfx->vbo);
    glGenVertexArrays(countof(gfx->vao), gfx->vao);
    glGenTextures(countof(gfx->texture), gfx->texture);
    glGenFramebuffers(3, gfx->fb);
    glGenRenderbuffers(1, &gfx->screendepth);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_post]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(gfx->vao[vo_post]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_BYTE, GL_FALSE, 2, 0);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_2d]);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(vert2d_t), 0, GL_STREAM_DRAW);
    glBindVertexArray(gfx->vao[vo_2d]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 4, GL_SHORT, GL_FALSE, 16, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 16, (void*)8);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, (void*)12);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_particle]);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(particle_t), 0, GL_STREAM_DRAW);
    glBindVertexArray(gfx->vao[vo_particle]);
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 4, GL_UNSIGNED_INT, 16, 0);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_water]);
    glBufferData(GL_ARRAY_BUFFER, 64 * sizeof(watervert_t), 0, GL_STREAM_DRAW);
    glBindVertexArray(gfx->vao[vo_water]);
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, 8, 0);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_strip]);
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(stripvert_t), 0, GL_STREAM_DRAW);
    glBindVertexArray(gfx->vao[vo_strip]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 32, (void*)12);
    glVertexAttribIPointer(2, 2, GL_UNSIGNED_INT, 32, (void*)24);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_gs]);
    glBufferData(GL_ARRAY_BUFFER, 256 * sizeof(gsvert_t), 0, GL_STREAM_DRAW);
    glBindVertexArray(gfx->vao[vo_gs]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 16, (void*)12);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_mapc]);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(mapcvert_t), 0, GL_STREAM_DRAW);
    glBindVertexArray(gfx->vao[vo_mapc]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 16, 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 16, (void*)4);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 16, (void*)8);

    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_font]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_text);

    gfx_size(gfx, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, gfx->fb[fb_screen]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           gfx->texture[tex_screen], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                           gfx->texture[tex_outline], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gfx->screendepth);

    void *data = file_raw("ui");
    if (data) {
        glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_ui]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    assert(glGetError() == 0);
    return 1;

cleanup:
    gfx_cleanup(gfx);
    return 0;
}

void gfx_cleanup(gfx_t *gfx)
{
    int i;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (i = 0; i < num_shader; i++)
        glDeleteProgram(gfx->shader[i].prog);

    glDeleteBuffers(countof(gfx->vbo), gfx->vbo);
    glDeleteTextures(countof(gfx->texture), gfx->texture);
    glDeleteRenderbuffers(1, &gfx->screendepth);
    glDeleteFramebuffers(countof(gfx->fb), gfx->fb);

    for (i = 0; i < countof(gfx->model); i++)
        free(gfx->model[i]);
}

void gfx_texture(gfx_t *gfx, const void *data, int width, int height, int i)
{
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_mdl + i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void gfx_model(gfx_t *gfx, const void *data, int i, int len, int nindex, int nvert)
{
    void *p;

    p = malloc(len);
    if (!p)
        return;

    memcpy(p, data, len); data += len;
    gfx->model[i] = p;

    glBindVertexArray(gfx->vao[vo_mdl + i]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gfx->vbo[vo_mdl_elements + i]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nindex * 2, data, GL_STATIC_DRAW);
    data += (((nindex + 1) & ~1) * 2);

    glBindBuffer(GL_ARRAY_BUFFER, gfx->vbo[vo_mdl + i]);
    glBufferData(GL_ARRAY_BUFFER, nvert * 24, data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, 0);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, 24, (void*)12);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, 24, (void*)16);
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_BYTE, 24, (void*)20);
}

void gfx_maptexture(gfx_t *gfx, const void *data)
{
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_map]);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 6); /* (1<<6 == 64) */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    //glGenerateMipmap(GL_TEXTURE_2D);
}

void gfx_particletexture(gfx_t *gfx, const void *data)
{
    glBindTexture(GL_TEXTURE_2D, gfx->texture[tex_particle]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32 * 64, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

void gfx_loadparticles(gfx_t *gfx, int count)
{
    int i;
    GLuint prog;
    const particledef_t *def;
    vec4 data[count];

    prog = gfx->shader[s_particle].prog;
    glUseProgram(prog);

    for (i = 0; i < count; i++) {
        def = &particledef[i];
        data[i] = scale4(vec4(def->color[0], def->color[1], def->color[2], def->color[3]), 1.0 / 255.0);
    }
    glUniform4fv(glGetUniformLocation(prog, "color"), count, &data[0].x);

    for (i = 0; i < count; i++) {
        def = &particledef[i];
        data[i] = vec4(def->start_frame, def->end_frame - def->start_frame, def->lifetime, 0.0);
    }
    glUniform4fv(glGetUniformLocation(prog, "info"), count, &data[0].x);
}

