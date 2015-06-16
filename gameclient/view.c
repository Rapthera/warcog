#include "view.h"

#define NEAR 1.0
#define FAR 2048.0
#define TANG 1.0

#include <stdbool.h>
#include <math.h>

void view_screen(view_t *view, float width, float height)
{
    view->width = width;
    view->height = height;
    view->aspect = width / height;
    view->matrix2d[0] = 2.0 / width;
    view->matrix2d[1] = -2.0 / height;
}

void view_params(view_t *view, float x, float y, float zoom, float theta)
{
    float sinx, cosx;
    ref_t ref;
    float a, b, c, d, j, k;
    matrix4_t m;

    sinx = sin(theta);
    cosx = cos(theta);

    /* reference */
    ref.pos = vec3(x, y + zoom * sinx, zoom * cosx);
    ref.x = vec3(1.0, 0.0, 00);
    ref.y = vec3(0.0, cosx, -sinx);
    ref.z = vec3(0.0, -sinx, -cosx);

    view->ref = ref;

    /* matrix */
    a = NEAR / (view->aspect * TANG);
    b = NEAR / TANG;
    c = -(FAR + NEAR) / (FAR - NEAR);
    d = -2.0 * FAR * NEAR / (FAR - NEAR);

    j = ref.pos.y * cosx - ref.pos.z * sinx;
    k = ref.pos.z * cosx + ref.pos.y * sinx;
    m.v[0] = vec4(a, 0.0, 0.0, 0.0);
    m.v[1] = vec4(0.0, b * cosx, c * sinx, -sinx);
    m.v[2] = vec4(0.0, b * -sinx, c * cosx, -cosx);
    m.v[3] = vec4(a * -ref.pos.x, b * -j, c * -k + d, k);

    view->matrix = m;

}

static matrix4_t matrix4_mul(matrix4_t a, matrix4_t b)
{
    matrix4_t res;
    int i, j;
    for(i = 0; i < 4; i++)
        for(j = 0; j < 4; j++)
            res.m[i][j] =  a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] +
                            a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];

    return res;
}

matrix4_t view_object(const view_t *view, vec3 pos, float angle, float s)
{
    matrix4_t m;
    float cosz, sinz;

    cosz = cos(angle);
    sinz = sin(angle);

    m.v[0] = vec4(s * cosz, s * sinz, 0.0, 0.0);
    m.v[1] = vec4(s * -sinz, s * cosz, 0.0, 0.0);
    m.v[2] = vec4(0.0, 0.0, s, 0.0);
    m.v[3] = vec4(pos.x, pos.y, pos.z, 1.0);

    return matrix4_mul(m, view->matrix);
}

vec2 view_point(const view_t *view, vec3 pos)
{
    vec3 v;

    v = ref_transform(sub3(pos, view->ref.pos), view->ref);
    return vec2(v.x / (v.z * TANG * view->aspect), v.y / (v.z * TANG));
}

static void intersect(bool *res, ref_t ref, vec3 pos, float aspect, vec2 sx, vec2 sy)
{
    float aux;
    vec3 p;

    p = ref_transform(pos, ref);

    res[0] = p.z > FAR;
    res[1] = p.z < NEAR;

    aux = p.z * TANG;
    res[2] = p.y > aux * sy.max;
    res[3] = p.y < aux * sy.min;

    aux = aux * aspect;
    res[4] = p.x > aux * sx.max;
    res[5] = p.x < aux * sx.min;
}

int view_intersect_box(const view_t *view, vec3 min, vec3 max, vec2 sx, vec2 sy)
{
    vec3 p;
    bool result[8][6];
    int i, j, out, in;

    min = sub3(min, view->ref.pos);
    max = sub3(max, view->ref.pos);

    p = min;
    intersect(result[0], view->ref, p, view->aspect, sx, sy);

    p.x = max.x;
    intersect(result[1], view->ref, p, view->aspect, sx, sy);

    p.y = max.y;
    intersect(result[2], view->ref, p, view->aspect, sx, sy);

    p.x = min.x;
    intersect(result[3], view->ref, p, view->aspect, sx, sy);

    p.z = max.z;
    intersect(result[4], view->ref, p, view->aspect, sx, sy);

    p.x = max.x;
    intersect(result[5], view->ref, p, view->aspect, sx, sy);

    p.y = min.y;
    intersect(result[6], view->ref, p, view->aspect, sx, sy);

    p.x = min.x;
    intersect(result[7], view->ref, p, view->aspect, sx, sy);

    in = 0;
    for (i = 0; i < 6; i++) {
        out = 0;
        for (j = 0; j < 8; j++)
            out += result[j][i];

        if (out == 8)
            return -1;

        in += out;
    }

    return (in == 0);
}

vec3 view_ray(const view_t *view, int x, int y)
{
    return add3_3(scale3(view->ref.z, NEAR),
                  scale3(view->ref.y, TANG * ((float) y * view->matrix2d[1] + 1.0)),
                  scale3(view->ref.x, TANG * view->aspect * ((float) x * view->matrix2d[0] - 1.0)));
}
