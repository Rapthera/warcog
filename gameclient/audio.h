#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include "audio/sample.h"
#include "audio/hrtf.h"

#define audio_samplerate 44100
#define audio_buffersize 512

/* sound formats */
enum {
    PCM16_2205,
};

typedef struct {
    volatile bool lock;
    volatile uint8_t sound;
    volatile bool playing, moving, done, stop;
    volatile vec3 pos;

    hrtf_params_t hrtf_params;
    hrtf_state_t hrtf_state;

    int i;
    uint32_t offset, counter;
    float lastgain;
    vec3 lastdir;
} source_t;

typedef struct {
    uint8_t format;
    int samples;
    const void *data;
} sound_t;

typedef struct {
    source_t source[256];
    int nsource;
    uint8_t i;

    volatile bool lock;
    volatile ref_t ref;

    sound_t sound[256];
} audio_t;

audio_t audio;

/* written for use with 2 threads,
    -1 calling only audio_getsamples()
    -1 calling the other functions
*/

/* render some samples */
void audio_getsamples(audio_t *a, sample_t *buf, int samples);

/* set listener reference */
void audio_listener(audio_t *a, ref_t ref);
/* start playing a sound, return source's id, can return -1 */
int audio_play(audio_t *a, int sound_id, vec3 pos, bool loop);
/* update a source's position
 if returns true, sound has finished playing and id is no longer valid */
bool audio_move(audio_t *a, int id, vec3 pos);
/* stop playing a source */
void audio_stop(audio_t *a, int id);

/* init audio, assumes zeroed-out */
void audio_init(audio_t *a);
/* load a sound */
void audio_load(audio_t *a, sound_t sound, int id);

#endif
