#include "entity.h"

#include <string.h>
#include "audio.h"
#include "map.h"
#include "particle.h"
#include "gfx.h"
#include "model.h"

//TODO: dont play sounds at feet

static void effect_netframe(entity_t *ent, effect_t *effect, uint16_t id, uint8_t delta)
{
    const effectdef_t *def;
    const particledef_t *pdef;
    int i;
    effectid_t d;
    vec3 pos;

    def = &effectdef[id];

    if (effect->start) {
        for (i = 0; i < def->count; i++) {
            d = def->id[i];
            switch (d.type) {
            case effect_particles:
                effect->var[i] = 0;
                break;
            case effect_strip:
                effect->var[i] = stripdef[d.i].lifetime + 1;
                break;
            case effect_sound:
                effect->var[i] = audio_play(&audio, sounddef[d.i].sound, ent->pos, 0);
                break;
            case effect_groundsprite:
                effect->var[i] = groundspritedef[d.i].lifetime + 1;
                break;
            }
        }
        effect->start = 0;
        return;
    }

    for (i = 0; i < def->count; i++) {
        d = def->id[i];
        switch (d.type) {
        case effect_particles:
            pdef = &particledef[d.i];
            effect->var[i] += delta;
            if (effect->var[i] >= pdef->rate) {
                effect->var[i] -= pdef->rate;
                pos = entity_bonepos(ent, pdef->bone);
                pos = add3(scale3(pos, 65536.0), vec3(0.5, 0.5, 0.5));
                particles_new(&particles, pos.x, pos.y, pos.z, d.i);
            }
            break;
        case effect_strip:
        case effect_groundsprite:
            if (effect->var[i] >= delta)
                effect->var[i] -= delta;
            else
                effect->var[i] = 0;
            break;
        case effect_sound:
            if (effect->var[i] >= 0 && audio_move(&audio, effect->var[i], ent->pos))
                effect->var[i] = -1;
            break;
        }
    }
}

static void effect_clear(entity_t *ent, effect_t *effect, uint16_t id)
{
    const effectdef_t *def;
    int i;
    effectid_t d;

    def = &effectdef[id];
    for (i = 0; i < def->count; i++) {
        d = def->id[i];
        switch (d.type) {
        case effect_sound:
            if (effect->var[i] >= 0)
                audio_stop(&audio, effect->var[i]);
            break;
        }
    }
}

static vec3 entity_pos(const entity_t *ent)
{
    return vec3((float) ent->x / 65536.0, (float) ent->y / 65536.0, map_height(&map, ent->x, ent->y));
}

vec3 entity_bonepos(const entity_t *ent, uint8_t bone)
{
    const entitydef_t *def;
    vec3 res;

    def = &entitydef[ent->def];

    if (def->model == 0xFFFF || bone == 0xFF)
        return ent->pos;

    res = model_gettrans(gfx.model[def->model], def->anim[ent->anim],
                         (double) ent->anim_frame / ent->anim_len, bone);
    res = add3(ent->pos, scale3(res, def->modelscale));
    /* TODO: rotation */
    return res;
}

void entity_clear(entity_t *ent)
{
    state_t *s;
    const entitydef_t *def;
    const statedef_t *sdef;
    int k;

    def = &entitydef[ent->def];

    for (k = 0; k < ent->nstate; k++) {
        s = &ent->state[k];
        sdef = &statedef[s->def];

        if (sdef->effect != 0xFFFF)
            effect_clear(ent, &s->effect, sdef->effect);
    }

    if (def->effect != 0xFFFF)
        effect_clear(ent, &ent->effect, def->effect);

    if (ent->voicesound >= 0)
        audio_stop(&audio, ent->voicesound);

    memset(ent, 0, sizeof(*ent));
    ent->voicesound = -1;
    ent->def = 0xFFFF;
}

entity_t* ents_get(ents_t *ents, int id)
{
    return &ents->ent[id];
}

void ents_netframe(ents_t *ents, uint8_t delta)
{
    entity_t *ent;
    ability_t *a;
    state_t *s;
    const entitydef_t *def;
    //const abilitydef_t *adef;
    const statedef_t *sdef;
    int i, j, k;

    for (i = 0, j = 0; i < 0xFFFF; i++) {
        ent = &ents->ent[i];
        def = &entitydef[ent->def];
        if (ent->def == 0xFFFF)
            continue;

        /* 3d position */
        ent->pos = entity_pos(ent);

        /* animation */
        if (ent->anim_len && !def->lockanim)
            ent->anim_frame = (ent->anim_frame + delta) % ent->anim_len;

        /* ability cooldowns */
        for (k = 0; k < ent->nability; k++) {
            a = &ent->ability[k];
            //adef = &abilitydef[a->def];
            a->cooldown = (a->cooldown <= delta) ? 0 : (a->cooldown - delta);

            //if (adef->effect != 0xFFFF)
                //effect_netframe(ent, &a->effect, adef->effect, delta);
        }

        /* state durations/effects */
        for (k = 0; k < ent->nstate; k++) {
            s = &ent->state[k];
            sdef = &statedef[s->def];
            s->duration = (s->duration <= delta) ? 0 : (s->duration - delta);
            s->age += delta;

            if (sdef->effect != 0xFFFF)
                effect_netframe(ent, &s->effect, sdef->effect, delta);
        }

        /* effect */
        if (def->effect != 0xFFFF)
            effect_netframe(ent, &ent->effect, def->effect, delta);

        /* voice sound */
        if (ent->voicesound >= 0)
            if (audio_move(&audio, ent->voicesound, ent->pos))
                ent->voicesound = -1;

        /* list */
        ents->list[j++] = i;
    }
    ents->count = j;
}

void ents_gfxframe(ents_t *ents, double delta)
{
    /* TODO: smooth anim, position, turning */
}

void ents_clear(ents_t *ents)
{
    entity_t *ent;
    int i;

    for (i = 0; i < ents->count; i++) {
        ent = &ents->ent[ents->list[i]];
        entity_clear(ent);
    }
}

void ents_init(ents_t *ents)
{
    int i;
    for (i = 0; i < 65536; i++)
        ents->ent[i].def = 0xFFFF;
}
