#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include "shader.h"
#include "math.h"
#include "gl.h"

#define maxbones 62

void model_draw(const shader_t *s, GLuint utrans, GLuint uquats,
                const GLuint *texture, const uint32_t *mdl,
                int anim_id, double frame, const float *teamcolor,
                const float *color, uint16_t tex_over);

vec3 model_gettrans(const uint32_t *mdl, int anim_id, double frame, int bone);

int model_load(const void *mdl, uint16_t *res, int *plen);

#endif
