#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>
#include "gl.h"

typedef struct {
    GLuint prog;
    GLint matrix, k, samp;
} shader_t;

bool shader_load(shader_t *res, const char *verts, const char *geoms, const char *frags);

#endif
