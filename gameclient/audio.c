#include "audio.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

//TODO: distance attenuation
//better "stop" (no cut-off)
//loading lock

static void lock(volatile bool *_lock)
{
    while (__sync_lock_test_and_set(_lock, 1))
        while (*_lock);
}

static void unlock(volatile bool *_lock)
{
    __sync_lock_release(_lock);
}

/* read samples to float 44.1k */
static void readsamples(float *dest, sound_t *sound, int i, int nsample)
{
    int max;
    const int16_t *data;

    data = sound->data;
    for (max = i + nsample; i < max; i += 2) {
        dest[0] = dest[1] = (float) data[i / 2] / 32768.0;
        dest += 2;
    }
}

static void mixsource(audio_t *a, ref_t ref, source_t *src, sample_t *buf, int nsample)
{
    int i;
    vec3 dir, pos;
    float distance, ev, az, delta, dirfact = 1.0, gain = 1.0;
    float samples[nsample];
    sound_t *sound;

    if (!src->playing)
        return;

    if (src->stop) {
        src->playing = 0;
        return;
    }

    if (src->done)
        return;

    lock(&src->lock);
    pos = src->pos;
    unlock(&src->lock);

    dir = ref_transform(sub3(pos, ref.pos), ref);
    distance = sqrt(dot3(dir, dir));
    if (distance > 0.0) {
        dir = scale3(dir, 1.0 / distance);
        ev = asin(dir.z);
        az = atan2(dir.x, -dir.y);
    } else {
        dir.x = 0.0; dir.y = 0.0; dir.z = -1.0;
        ev = 0.0; az = 0.0;
    }

    if (src->moving) {
        delta = hrtf_calcfadetime(src->lastgain, gain, &src->lastdir, &dir);
        if (delta > 0.000015) {
            src->counter = hrtf_moving(&src->hrtf_params, ev, az, dirfact, gain, delta, src->counter);
            src->lastgain = gain;
            src->lastdir = dir;
        }
    } else {
        hrtf_lerped(&src->hrtf_params, ev, az, dirfact, gain);
        memset(&src->hrtf_state, 0, sizeof(src->hrtf_state));

        src->i = 0;
        src->offset = 0;
        src->counter = 0;
        src->lastgain = gain;
        src->lastdir = dir;

        src->moving = 1;
    }

    sound = &a->sound[src->sound];

    i = sound->samples - src->i;
    if (i < nsample) {
        if (i > 0) {
            readsamples(samples, sound, src->i, i);
            memset(&samples[i], 0, (nsample - i) * sizeof(*samples));
        } else {
            memset(samples, 0, nsample * sizeof(*samples));
            if (src->i >= sound->samples + HRTF_HISTORY_LENGTH)
                src->done = 1;
        }
    } else {
        readsamples(samples, sound, src->i, nsample);
    }
    src->i += nsample;

    hrtf_mix(buf, samples, src->counter, src->offset, 0, &src->hrtf_params, &src->hrtf_state, nsample);
    src->offset += nsample;
    if (src->counter < nsample)
        src->counter = 0;
    else
        src->counter -= nsample;
}

void audio_getsamples(audio_t *a, sample_t *buf, int samples)
{
    int i;
    ref_t ref;

    memset(buf, 0, samples * sizeof(*buf));

    lock(&a->lock);
    ref = a->ref;
    unlock(&a->lock);

    for (i = 0; i < 256; i++)
        mixsource(a, ref, &a->source[i], buf, samples);
}

void audio_listener(audio_t *a, ref_t ref)
{
    lock(&a->lock);
    a->ref = ref;
    unlock(&a->lock);
}

int audio_play(audio_t *a, int sound_id, vec3 pos, bool loop)
{
    int id;
    source_t *src;

    if (a->nsource == 256)
        return -1;
    a->nsource++;

    while ((src = &a->source[id = a->i++])->playing); /* find a free source */

    src->sound = sound_id;
    src->pos = pos;
    src->stop = 0;
    src->done = 0;
    src->moving = 0;
    src->playing = 1;

    return id;
}

bool audio_move(audio_t *a, int id, vec3 pos)
{
    source_t *src;

    src = &a->source[id];
    if (src->done) {
        src->playing = 0;
        a->nsource--;
        return 1;
    }

    lock(&src->lock);
    src->pos = pos;
    unlock(&src->lock);
    return 0;
}

void audio_stop(audio_t *a, int id)
{
    a->source[id].stop = 1;
    a->nsource--;
}

void audio_init(audio_t *a)
{
}

void audio_load(audio_t *a, sound_t sound, int id)
{
    a->sound[id] = sound;
}
