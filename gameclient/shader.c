#include "shader.h"

#include <stdio.h>
#include <stdlib.h>

static void debug_infolog(GLuint shader, const char *data)
{
    #ifndef NDEBUG
    GLint infologsize;
    char* infolog;

    printf("shader loading failed:\n%s\n", data);
    infologsize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologsize);
    if (infologsize) {
        infolog = malloc(infologsize);
        glGetShaderInfoLog(shader, infologsize, NULL, (void*)infolog);
        printf("Infolog: %s\n", infolog);
        free(infolog);
    }
    #endif
}

static bool compile_shader(GLuint shader, const char *data)
{
    GLint status;

    glShaderSource(shader, 1, &data, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        debug_infolog(shader, data);
        return 0;
    }

    return 1;
}

bool shader_load(shader_t *res, const char *verts, const char *geoms, const char *frags)
{
    GLuint prog, vert, geom, frag;
    GLint status;

    vert = glCreateShader(GL_VERTEX_SHADER);
    if (!vert)
        return 0;

    if (!compile_shader(vert, verts))
        goto EXIT_DELETE_VERT;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    if (!frag)
        goto EXIT_DELETE_VERT;

    if (!compile_shader(frag, frags))
        goto EXIT_DELETE_FRAG;

    if (geoms) {
        geom = glCreateShader(GL_GEOMETRY_SHADER);
        if (!geom)
            goto EXIT_DELETE_FRAG;

        if (!compile_shader(geom, geoms))
        goto EXIT_DELETE_GEOM;
    }

    prog = glCreateProgram();
    if (!prog)
        goto EXIT_DELETE_GEOM;


    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    if (geoms)
        glAttachShader(prog, geom);

    glBindFragDataLocation(prog, 0, "c");
    glBindFragDataLocation(prog, 1, "e");

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        debug_infolog(prog, "glLinkProgram");
        glDeleteProgram(prog);
        goto EXIT_DELETE_GEOM;
    }

    res->prog = prog;
    res->matrix = glGetUniformLocation(prog, "matrix");
    res->k = glGetUniformLocation(prog, "k");
    res->samp = glGetUniformLocation(prog, "samp");

    if (geoms) {
        glDetachShader(prog, geom);
        glDeleteShader(geom);
    }

    glDetachShader(prog, frag);
    glDeleteShader(frag);
    glDetachShader(prog, vert);
    glDeleteShader(vert);
    return 1;

EXIT_DELETE_GEOM:
    if (geoms)
        glDeleteShader(geom);
EXIT_DELETE_FRAG:
    glDeleteShader(frag);
EXIT_DELETE_VERT:
    glDeleteShader(vert);
    return 0;
}
