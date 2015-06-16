#ifndef VIEW_H
#define VIEW_H

#include "math.h"

typedef struct {
    float width, height, aspect;
    float matrix2d[2];
    matrix4_t matrix;
    ref_t ref;
} view_t;

view_t view;

/* update screen parameters
 doesnt update 3d view, view_params() must be called again */
void view_screen(view_t *view, float width, float height);
/* update 3d view with some parameters (TODO: more parameters) */
void view_params(view_t *view, float x, float y, float zoom, float theta);

/* complete matrix for object with position, angle (z), and scale */
matrix4_t view_object(const view_t *view, vec3 pos, float angle, float s);
/* 3d point to screen coords */
vec2 view_point(const view_t *view, vec3 pos);
/* check if box (min, max) intersects screen area (smin, smax)
 -1 = completely outside, 0 = intersecting, 1 = completely inside */
int view_intersect_box(const view_t *view, vec3 min, vec3 max, vec2 smin, vec2 smax);
/* get ray from screen coordinates (x,y) in pixels */
vec3 view_ray(const view_t *view, int x, int y);

#endif
