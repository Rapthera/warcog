#include "input.h"

#include <stdlib.h>
#include <math.h>
#include "entity.h"
#include "map.h"
#include "view.h"
#include "keycodes.h"
#include "net.h"
#include "audio.h"
#include "gfx.h"

#define SS_KEY (3.0) /* units per second per distance */
#define SS_MOUSE_EDGE (3.0) /* units per second per distance */
#define SS_MOUSE (0.375 / 128.0) /* units per pixel per distance */

static void update_over(input_t *in, const view_t *view, const ents_t *ents, const map_t *map,
                        int x, int y)
{
    int i;
    uint16_t id;

    const entity_t *en;
    const entitydef_t *def;

    float f, d;
    vec3 ray, ray_inv;
    vec3 min, max;

    tree_t tree[map->size_shift * 3 + 1], *t;
    maptree_t *mt;
    int size;
    vec3 res;

    in->target = 0;
    in->target_ent = 0xFFFF;

    ray = view_ray(view, x, y);
    ray_inv = inv3(ray);

    d = INFINITY;
    for (i = 0; i < ents->count; i++) {
        en = &ents->ent[id = ents->list[i]];
        def = &entitydef[en->def];

        if (!def->target)
            continue;

        min = sub3(en->pos, vec3(def->boundsradius, def->boundsradius, 0.0));
        max = add3(en->pos, vec3(def->boundsradius, def->boundsradius, def->boundsheight));

        f = ray_intersect_box(view->ref.pos, ray_inv, min, max);
        if (f < d) {
            d = f;
            in->target |= tf_unit;
            in->target_ent = id;
        }
    }

    if (!map->size)
        return;

    d = INFINITY;
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

        f = ray_intersect_box(view->ref.pos, ray_inv, min, max);
        if (f == INFINITY) { // if (f >= d)
            t--;
        } else if (t->depth == map->size_shift) {
            res = map_intersect_tile(map, view->ref.pos, ray, t->x, t->y);
            if (res.z < d) {
                d = res.z;
                in->target |= tf_ground;
                in->tx = (res.x * 65536.0);
                in->ty = (res.y * 65536.0);
            }
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
}

static void update_selection(input_t *in, const view_t *view, const ents_t *ents, int x, int y)
{
    int i, j;
    uint16_t id;

    const entity_t *en;
    const entitydef_t *def;
    selection_t *sel;
    vec2 sx, sy;
    vec3 min, max;

    sel = &in->sel;
    sel->x2 = x;
    sel->y2 = y;

    sx = add2(scale2((x >= sel->x) ? vec2(sel->x, x + 1) : vec2(x, sel->x + 1), view->matrix2d[0]),
         vec2(-1.0, -1.0));

    sy = add2(scale2((y < sel->y) ? vec2(sel->y, y + 1) : vec2(y, sel->y + 1), view->matrix2d[1]),
         vec2(1.0, 1.0));

    for (j = 0, i = 0; i < ents->count; i++) {
        en = &ents->ent[id = ents->list[i]];
        def = &entitydef[en->def];

        if (en->owner != self_id() || !def->control)
            continue;

        min = sub3(en->pos, vec3(def->boundsradius, def->boundsradius, 0.0));
        max = add3(en->pos, vec3(def->boundsradius, def->boundsradius, def->boundsheight));

        if (view_intersect_box(view, min, max, sx, sy) < 0)
            continue;

        sel->ent[j++] = id;
        if (j == 256)
            break;
    }
    sel->count = j;
}

void input_validsel(input_t *in)
{
    int i, j, id;
    selection_t *sel;
    const entity_t *en;

    sel = &in->sel;
    for (j = 0, i = 0; i < sel->count; i++) {
        en = ents_get(&ents, id = sel->ent[i]);
        if (en->def == 0xFFFF)
            continue;

        sel->ent[j++] = id;
    }
    sel->count = j;
}

static void play_voice(input_t *in, audio_t *a, entity_t *ent, int id)
{
    const entitydef_t *def;
    const voice_t *v;

    def = &entitydef[ent->def];
    v = &def->voice[id];

    if (in->voice_cd > 0)
        return;

    if (ent->voicesound >= 0)
        audio_stop(a, ent->voicesound);

    ent->voicesound = audio_play(a, v->start + rand() %  v->len, ent->pos, 0);
    in->voice_cd = 2.0;
}

static int flags(input_t *in, uint8_t target)
{
    return (in->shift << 4) | target;
}

static void order(input_t *in, uint8_t action, uint8_t target, uint16_t id, uint32_t x, uint32_t y)
{
    net_order(&net, action, flags(in, target), id, x, y);
}

static void order_target(input_t *in, uint8_t action, uint8_t target, uint16_t id, uint8_t flags)
{
    if (target & tf_unit) {
        order(in, action, target_unit | flags, id, in->target_ent, 0);
    } else if (target & tf_ground) {
        order(in, action, target_ground | flags, id, in->tx, in->ty);
        in->pradius = 8.0;
        in->px = in->tx;
        in->py = in->ty;
    } else {
        return;
    }

    play_voice(in, &audio, ents_get(&ents, id), 1);
}

void input_keydown(input_t *in, uint32_t keycode, uint32_t unicode)
{
    selection_t *sel;
    entity_t *ent;
    ability_t *a;
    const abilitydef_t *adef;
    int k;

    switch (keycode) {
    case KEY_A ... KEY_Z:
        keycode -= KEY_A;
        sel = &in->sel;
        if (sel->count) {
            ent = ents_get(&ents, sel->ent[0]);
            for (k = 0; k < ent->nability; k++) {
                a = &ent->ability[k];
                adef = &abilitydef[a->def];

                if (adef->key == keycode) {
                    if (adef->target == tf_self)
                        order(in, k, target_self | (adef->queue << 5), sel->ent[0], 0, 0);
                    else
                        in->ability = a->def;
                    break;
                }
            }

            if (keycode == KEY_G - KEY_A)
                order(in, 129, target_self | (1 << 5), sel->ent[0], 0, 0);
        }
        break;
    case KEY_UP:
        in->up = 1;
        break;
    case KEY_DOWN:
        in->down = 1;
        break;
    case KEY_LEFT:
        in->left = 1;
        break;
    case KEY_RIGHT:
        in->right = 1;
        break;
    case KEY_SHIFT:
        in->shift = 1;
        break;
    }
}

void input_keyup(input_t *in, uint32_t keycode)
{
    switch (keycode) {
    case KEY_UP:
        in->up = 0;
        break;
    case KEY_DOWN:
        in->down = 0;
        break;
    case KEY_LEFT:
        in->left = 0;
        break;
    case KEY_RIGHT:
        in->right = 0;
        break;
    case KEY_SHIFT:
        in->shift = 0;
        break;
    }
}

void input_mmove(input_t *in, int x, int y, uint8_t id)
{
    int dx, dy;

    dx = in->mx - x;
    dy = in->my - y;
    in->mx = x;
    in->my = y;

    update_over(in, &view, &ents, &map, x, y);

    if (in->sel.active) {
        update_selection(in, &view, &ents, x, y);
        return;
    }

    if (in->middle) {
        in->cx += (double) dx * in->zoom * SS_MOUSE;
        in->cy -= (double) dy * in->zoom * SS_MOUSE;
    }
}

void input_mdown(input_t *in, int x, int y, uint8_t id, uint8_t button)
{
    selection_t *sel;
    entity_t *ent;
    const abilitydef_t *adef;
    uint8_t target;
    int k;

    sel = &in->sel;

    switch (button) {
    case 0:
        if (in->ability != 0xFFFF && sel->count) {
            adef = &abilitydef[in->ability];
            target = in->target & adef->target;
            if (target) {
                ent = ents_get(&ents, sel->ent[0]);
                for (k = 0; k < ent->nability; k++) {
                    if (ent->ability[k].def == in->ability) {
                        order_target(in, k, target, sel->ent[0], 0);
                        break;
                    }
                }
            }
            in->ability = 0xFFFF;
            break;
        }

        sel->active = 1;
        sel->count = 0;
        sel->x = x;
        sel->y = y;

        if (in->target_ent != 0xFFFF && ents_get(&ents, sel->ent[0])->owner == self_id()) {
            sel->count = 1;
            sel->ent[0] = in->target_ent;
        }

        update_selection(in, &view, &ents, x, y);
        break;
    case 1:
        in->middle = 1;
        break;
    case 2:
        in->ability = 0xFFFF;
        target = in->target & (tf_ground);
        if (sel->count)
            order_target(in, 128, target, sel->ent[0], 0);
        break;
    }
}

void input_mup(input_t *in, int x, int y, uint8_t id, int8_t button)
{
    switch (button) {
    case 0:
        if (!in->sel.active)
            break;

        in->sel.active = 0;

        if (in->sel.count)
            play_voice(in, &audio, &ents.ent[in->sel.ent[0]], 0);
        break;
    case 1:
        in->middle = 0;
        break;
    case 2:
        break;
    }
}

void input_mleave(input_t *in, uint8_t id)
{
    in->mx = -1;
    in->my = -1;
}

void input_mwheel(input_t *in, double delta)
{
    in->zoom -= delta * 16.0;
}

void input_frame(input_t *in, double delta)
{
    double k;

    k = delta * in->zoom * SS_KEY;
    if (in->right)
        in->cx += k;
    if (in->left)
        in->cx -= k;
    if (in->up)
        in->cy += k;
    if (in->down)
        in->cy -= k;

    k = delta * in->zoom * SS_MOUSE_EDGE;
    if (!in->mx)
        in->cx -= k;
    if (in->mx + 1 == gfx.iwidth)
        in->cx += k;
    if (!in->my)
        in->cy += k;
    if (in->my + 1 == gfx.iheight)
        in->cy -= k;

    if (in->voice_cd > 0)
        in->voice_cd -= delta;

    if (in->pradius > 0)
        in->pradius -= delta * 32.0;

    view_params(&view, in->cx, in->cy, in->zoom, in->theta);

    update_over(in, &view, &ents, &map, in->mx, in->my);

    if (in->sel.active) {
        update_selection(in, &view, &ents, in->mx, in->my);
        return;
    }
}
