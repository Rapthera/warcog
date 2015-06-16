#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "audio.h"
#include "entity.h"
#include "view.h"
#include "gfx.h"
#include "input.h"
#include "net.h"
#include "map.h"

//TODO remove entities from selection in net

static void fps_count(double delta)
{
    static double total;
    static int fps;

    total += delta;
    if (total >= 1.0) {
        total -= 1.0;
        printf("FPS: %u\n", fps);
        fps = 0;
    }
    fps++;
}

void do_recv(const uint8_t *data, int len)
{
    net_recv(&net, data, len);
}

void do_msg(uint8_t msg, uint8_t v8, uint16_t v16, uint32_t value, void *data)
{
    int i;
    void *p;

    switch (msg) {
    case msg_loadparticles:
        gfx_loadparticles(&gfx, v16);
        break;
    case msg_loadtext:
        gfx.text = data;
        break;
    case msg_loadmap:
        map_load(&map, data, v8);
        free(data);
        break;
    case msg_loadtex:
        p = data;
        for (i = 0; i < v16; i++)
            gfx_texture(&gfx, p, 256, 256, i), p += 256 * 256 * 4;
        gfx_particletexture(&gfx, p), p += 256 * 256 * 4;
        gfx_maptexture(&gfx, p), p += 1024 * 1024 * 4;
        free(data);
        break;
    case msg_loadmdl:
        gfx_model(&gfx, data, v8, v16, value & 0xFFFF, value >> 16);
        free(data);
        break;
    case msg_loaded:
        net_loaded(&net);
        break;
    default: __builtin_unreachable();
    }
}

bool do_init(const config_t *cfg)
{
    if (!gfx_init(&gfx, cfg->width, cfg->height))
        return 0;

    ents_init(&ents);
    audio_init(&audio);
    net_init(&net, cfg->dlrate, cfg->name);
    //particle_init(&global.particle);

    input.target_ent = 0xFFFF;
    input.ability = 0xFFFF;
    input.theta = -M_PI * 1.0 / 16.0;
    input.zoom = 128.0;
    input.mx = -1;
    input.my = -1;

    view_screen(&view, cfg->width, cfg->height);

    /*//view_pos(&view, cam.x, cam.y, cam.zoom, cam.theta);
    //audio_listener(&audio, view.ref);

    global.player[255].color[0] = 0.0;
    global.player[255].color[1] = 0.0;
    global.player[255].color[2] = 0.0;
    global.player[255].color[3] = 1.0;
    global.sel.over = 0xFFFF;*/


    return 1;
}

void do_resize(int width, int height)
{
    view_screen(&view, width, height);
    gfx_size(&gfx, width, height);
}

void do_cleanup(void)
{
    //audio_cleanup
    net_cleanup(&net);
    gfx_cleanup(&gfx);
}

void do_frame(double delta)
{
    /*
    selection_t *sel = &global.sel;
    if (sel->ptime > 0.0)
        sel->ptime -= delta;
    if (global.voice_cooldown > 0.0)
        global.voice_cooldown -= delta;

    particle_frame(&global.particle);*/

    net_frame(&net, delta);

    input_frame(&input, delta);

    audio_listener(&audio, view.ref);

    gfx_draw(&gfx);

    fps_count(delta);
}
