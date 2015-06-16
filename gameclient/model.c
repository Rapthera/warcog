#include "model.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/* note: models originally had additional interpolation data */

typedef struct {
    uint8_t ngeo, teamcolor_end, nbone, nanim;
} header_t;

typedef struct {
    uint16_t index, texture;
} chunkinfo_t;

typedef struct {
	int	frame;
	vec3 vec;
} keyframe_t;

typedef struct {
	int	frame;
	vec4 vec;
} keyframe4_t;

typedef struct {
    int8_t parent;
    uint8_t keyframes[2], linetype;
    vec3 pivot;
} bone_t;

typedef struct {
    uint16_t vis, frames;
} anim_t;

static const float transparency[] = {0.0, 0.0, 0.0, 0.0};

static void get_trans(vec3 *res, const uint8_t *p, const uint8_t *tr, int anim_id, double frame)
{
    uint8_t start, end;
    const keyframe_t *k, *j;
    float t;

    start = (!anim_id) ? 0 : tr[anim_id - 1];
    end = tr[anim_id];
    k = (const keyframe_t*)p; k += start;

    if(start == end) {
        memset(res, 0, sizeof(*res));
        return;
    }

    if(start + 1 == end || frame <= k->frame) {
        *res = k->vec;
        return;
    }

    while((++k)->frame < frame);

    j = k - 1;
    t = (float)(frame - j->frame) / (float)(k->frame - j->frame);
    *res = lerp3(j->vec, k->vec, t);
}

static void get_rot(vec4 *res, const uint8_t *p, const uint8_t *qt, int anim_id, double frame)
{
    uint8_t start, end;
    const keyframe4_t *k, *j;
    float t;

    start = (!anim_id) ? 0 : qt[anim_id - 1];
    end = qt[anim_id];
    k = (const keyframe4_t*)p; k += start;

    if(start == end) {
        *res = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    if(start + 1 == end || frame <= k->frame) {
        *res = k->vec;
        return;
    }

    while((++k)->frame < frame);

    j = k - 1;
    t = (float)(frame - j->frame) / (float)(k->frame - j->frame);
    *res = lerp4(j->vec, k->vec, t);
}

#define align4(p) (void*)(((size_t)(p) + 3) & (~3))

void model_draw(const shader_t *s, GLuint utrans, GLuint uquats,
                const GLuint *texture, const uint32_t *mdl,
                int anim_id, double frame, const float *teamcolor,
                const float *color, uint16_t tex_over)
{
    /* assumes valid model data */
    uint8_t ngeo, teamcolor_end, nbone, nanim;
    const uint8_t *p, *tr, *qt;

    const chunkinfo_t *ck;
    const anim_t *anim;
    const bone_t *b;
    vec3 trans, ptrans;
    vec4 rot, prot;
    int vis, i, offset, tex;

    vec3 rtrans[maxbones];
    vec4 rotation[maxbones];

    /* parse header */
    p = (const uint8_t*)mdl;
    ngeo = p[0];
    teamcolor_end = p[1];
    nbone = p[2];
    nanim = p[3];

    anim = (const anim_t*)&p[4];
    ck = (const chunkinfo_t*)&anim[nanim];
    p = (const uint8_t*)&ck[ngeo];
    anim += anim_id;

    vis = anim->vis;
    frame = frame * anim->frames;

    /* calculate transformations for frame */
    i = 0;
    do {
        b = (const bone_t*)p;
        tr = (const uint8_t*)&b[1];
        qt = tr + nanim;
        p = qt + nanim;
        p = align4(p);

        get_trans(&trans, p, tr, anim_id, frame);
        p += b->keyframes[0] * sizeof(keyframe_t);

        get_rot(&rot, p, qt, anim_id, frame);
        p += b->keyframes[1] * sizeof(keyframe4_t);

        if(b->parent < 0) {
            ptrans = vec3(0.0, 0.0, 0.0);
            prot = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            ptrans = rtrans[b->parent];
            prot = rotation[b->parent];
        }

        rotation[i] = mul4(rot, prot);
        trans = add3(ptrans, qrot3(add3(b->pivot, trans), prot));
        rtrans[i] = add3(qrot3(neg3(b->pivot), rotation[i]), trans);
    } while (++i < nbone);

    /* load transformations */
    glUniform4fv(uquats, nbone, (float*)rotation);
    glUniform3fv(utrans, nbone, (float*)rtrans);

    //GLuint test;
    //glGenBuffers(1, &test);
    //
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, ck[ngeo - 1].index, indices, GL_STATIC_DRAW);

    /* draw visible vertices */
    i = 0;
    offset = 0;
    tex = 0xFFFF;
    glUniform4fv(s->k, 1, teamcolor);
    glUniform4fv(s->samp, 1, color);
    do {
        if(i == teamcolor_end)
            glUniform4fv(s->k, 1, transparency);

        if((vis & (1 << i))) {
            if (tex_over != 0xFFFF) {
                glBindTexture(GL_TEXTURE_2D, tex_over == 0xFFFE ? 0 : texture[tex_over]);
            } else if (ck[i].texture != tex) {
                tex = ck[i].texture;
                glBindTexture(GL_TEXTURE_2D, texture[tex]);
            }

            glDrawElements(GL_TRIANGLES, ck[i].index - offset, GL_UNSIGNED_SHORT, (void*)(size_t)offset);
        }
        offset = ck[i].index;
    } while(++i != ngeo);

    //glDeleteBuffers(1, &test);
}

/* todo: implement this correctly */
vec3 model_gettrans(const uint32_t *mdl, int anim_id, double frame, int bone)
{
    uint8_t ngeo, nbone, nanim;
    const uint8_t *p, *tr, *qt;

    const chunkinfo_t *ck;
    const anim_t *anim;
    const bone_t *b;
    vec3 trans, ptrans;
    vec4 rot, prot;
    int i;

    vec3 translation[maxbones];
    vec3 rtrans[maxbones];
    vec4 rotation[maxbones];

    if (bone == 0xFF)
        return vec3(0.0, 0.0, 0.0);

    /* parse header */
    p = (const uint8_t*)mdl;
    ngeo = p[0];
    nbone = p[2];
    nanim = p[3];

    anim = (const anim_t*)&p[4];
    ck = (const chunkinfo_t*)&anim[nanim];
    p = (const uint8_t*)&ck[ngeo];
    anim += anim_id;

    frame = frame * anim->frames;

    /* calculate transformations for frame */
    i = 0;
    do {
        b = (const bone_t*)p;
        tr = (const uint8_t*)&b[1];
        qt = tr + nanim;
        p = qt + nanim;
        if(nanim & 1) { /* alignment */
            p += 2;
        }

        get_trans(&trans, p, tr, anim_id, frame);
        p += b->keyframes[0] * sizeof(keyframe_t);

        get_rot(&rot, p, qt, anim_id, frame);
        p += b->keyframes[1] * sizeof(keyframe4_t);

        if(b->parent < 0) {
            ptrans = vec3(0.0, 0.0, 0.0);
            prot = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            ptrans = rtrans[b->parent];
            prot = rotation[b->parent];
        }

        rotation[i] = mul4(rot, prot);
        translation[i] = add3(ptrans, qrot3(add3(b->pivot, trans), prot));
        rtrans[i] = add3(qrot3(neg3(b->pivot), rotation[i]), translation[i]);

        if (i == bone)
            return translation[i];

    } while(++i < nbone);

    return vec3(0.0, 0.0, 0.0);
}

#define sublen(x) if ((len -= (x)) < 0) return 0;

int model_load(const void *mdl, uint16_t *res, int *plen)
{
    const header_t *h;
    const chunkinfo_t *ck;
    const uint16_t *indices;
    const bone_t *b;
    int i, k, nindex, nvert;
    int len;

    len = *plen;

    h = mdl;

    k = sizeof(header_t);
    sublen(k); mdl += k;

    if (!h->ngeo || !h->nbone || h->nbone > maxbones)
        return 0;

    k = sizeof(anim_t) * h->nanim;
    sublen(k); mdl += k;

    ck = mdl;

    k = sizeof(*ck) * h->ngeo;
    sublen(k); mdl += k;

    nindex = ck[h->ngeo - 1].index;

    i = 0;
    do {
        b = mdl;

        k = sizeof(bone_t) + ((h->nanim + 1) / 2) * 4;
        sublen(k); mdl += k;

        k = b->keyframes[0] * sizeof(keyframe_t) + b->keyframes[1] * sizeof(keyframe4_t);
        sublen(k); mdl += k;
    } while (++i < h->nbone);

    indices = mdl;

    k = sizeof(uint16_t) * nindex;
    sublen(k); mdl += k;

    for (nvert = 0, i = 0; i < nindex; i++)
        if (indices[i] >= nvert)
            nvert = indices[i] + 1;

    if (nindex & 1) {
        sublen(2);
        mdl += 2;
    }

    k = 24 * nvert;
    sublen(k); mdl += k;

    *res++ = ((const void*)indices - (const void*)h);
    *res++ = nindex;
    *res++ = nvert;
    *plen = len;
    return (mdl - (const void*)h);
}
