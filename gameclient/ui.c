#include "ui.h"

#include <stdio.h>

#include "input.h"
#include "gfx.h"
#include "net.h"

#define white rgba(255,255,255,255)
#define darken rgba(0,0,0,128)

static vert2d_t* loadingbar(vert2d_t *v, const char *str, double progress)
{
    const font_t *font;
    int x, y;

    font = &gfx.font[font_ui_big];

    v = textverts(v, font, str,
                  (gfx.iwidth - textwidth(font, str)) / 2,
                  (gfx.iheight / 2) - font->height - 2, white);

    x = (gfx.iwidth / 2) - 150;
    y = (gfx.iheight / 2);

    /* border */
    *v++ = vert2d_solid(x, y, 300, 2, white);
    *v++ = vert2d_solid(x, y + 2, 2, 26, white);
    *v++ = vert2d_solid(x + 298, y + 2, 2, 26, white);
    *v++ = vert2d_solid(x, y + 28, 300, 2, white);

    /* inside */
    *v++ = vert2d_solid(x + 4, y + 4, (292.0 * progress + 0.5), 22, white);

    return v;
}

static vert2d_t* cooldown(vert2d_t *v, int x, int y, int s, double val)
{
    *v++ = vert2d_solid(x, y, s * 16, ((s * 16) * val + 0.5), darken);
    return v;
}

vert2d_t* ui_verts(vert2d_t* v, const ents_t *ents, int mousex, int mousey)
{
    const entity_t *ent;
    const ability_t *ability, *a;
    const entitydef_t *def;
    const abilitydef_t *adef;
    const font_t *font;

    int i, nability, tmp;
    int hp, hp_max, mana, mana_max;
    char str[32];
    const char *text;

    selection_t *sel;
    int x, y, w, h, s, sw, sh;

    sw = gfx.iwidth;
    sh = gfx.iheight;
    s = sh / 192;
    if (s > sw / 256)
        s = sw / 256;

    sel = &input.sel;

    if (!net.connected)
        return loadingbar(v, "Connecting...", 0.0);

    if (net.map_parts_left)
        return loadingbar(v, "Downloading...",
                          (double) (net.map_parts - net.map_parts_left) / net.map_parts);

    if (!net.loaded)
        return loadingbar(v, "Loading...", 0.0);

    for (i = 0; i < ents->count; i++) {
        ent = &ents->ent[ents->list[i]];
    }

    if (sel->active) {
        if (sel->x < sel->x2)
            x = sel->x, w = sel->x2 - sel->x;
        else
            x = sel->x2, w = sel->x - sel->x2;

        if (sel->y < sel->y2)
            y = sel->y, h = sel->y2 - sel->y;
        else
            y = sel->y2, h = sel->y - sel->y2;

        *v++ = vert2d_solid(x, y, w, h, rgba(255, 255, 255, 128));
    }

    nability = 0;
    hp = 0; hp_max = 0;
    mana = 0; mana_max = 0;
    if (sel->count) {
        ent = &ents->ent[sel->ent[0]];
        def = &entitydef[ent->def];

        hp = ent->hp;
        hp_max = def->hp >> 16;
        mana = ent->mana;
        mana_max = def->mana >> 16;

        nability = ent->nability;
        ability = ent->ability;
    }

    x = sw / 2 - s * 61;
    y = sh - s * 36;
    font = &gfx.font[font_ui];

    *v++ = vert2d_solid(x, y + s * 5 / 2,
                        hp_max ? s * 121 * hp / hp_max : s * 121,
                        s * 5, rgba(0, 192, 0, 255));

    sprintf(str, "%u/%u\n", hp, hp_max);
    v = textverts(v, font, str,
                 x  + (s * 121 - textwidth(font, str)) / 2,
                 y + s * 7 / 2,
                 white);

    *v++ = vert2d_solid(x, y + s * 15 / 2,
                        mana_max ? s * 121 * mana / mana_max : s * 121,
                        s * 3, rgba(0, 0, 255, 255));

    sprintf(str, "%u/%u\n", mana, mana_max);
    v = textverts(v, font, str,
                 x + (s * 121 - textwidth(font, str)) / 2,
                 y + s * 8,
                 white);

    *v++ = vert2d_scale(x - s * 3, y, 1024, 288, 0, 736, s, white);

    y += s * 14;
    for (i = 0; i < nability; i++) {
        a = &ability[i];
        adef = &abilitydef[a->def];
        *v++ = vert2d_icon(x, y, adef->icon, s, white);
        *v++ = vert2d_scale(x, y, 128, 128, (i == 0) ? 0 : 128, 0, s, white);

        str[0] = adef->key + 'A'; str[1] = 0;
        v = textverts(v, font, str, x, y, white);

        if (a->cooldown) {
            if (a->cooldown >= adef->cooldown)
                *v++ = vert2d_solid(x, y, s * 16, s * 16, darken);
            else
                v = cooldown(v, x, y, s, (double) a->cooldown / adef->cooldown);

            tmp = a->cooldown + 3;
            sprintf(str, "%u.%us", tmp / 60, (tmp % 60) / 6);
            v = textverts(v, font, str, x + s * 16 - textwidth(font, str), y, white);
        }


        //*v++ = vert2d_solid(x, y, s * 8, s * 8, rgba(0, 128, 255, 255));
        x += s * 21;
    }

    x = mousex - (sw / 2 - s * 61);
    y = mousey - (sh - s * 22);
    if (x >= 0 && x < s * 121 && y >= 0 && y < s * 16) {
        i = x / (s * 21);
        x = x % (s * 21);
        if (i < nability && x < s * 16) {
            a = &ability[i];
            adef = &abilitydef[a->def];

            text = text(adef->description);
            h = textboxheight(font, text, s * 62) + s * 6;
            x = sw - s * 64;
            y = sh - s * 46 - h;

            *v++ = vert2d_solid(x, y, s * 64, h, darken);
            v = textverts(v, font, text(adef->name), x + s, y + s, white);
            v = textbox(v, font, text, x + s, y + s * 5, s * 62, white);
        }
    }

    *v++ = vert2d_scale(0, sh - s * 64, 512, 512, 0, 224, s, white);
    *v++ = vert2d_scale(sw - s * 64, sh - s * 46, 512, 368, 512, 368, s, white);

    return v;
}

