#include <stdbool.h>
#include <alsa/asoundlib.h>
#include "../audio.h"

volatile bool alsa_init, alsa_quit;

static snd_pcm_t* alsa_open(const char *dev_name)
{
    snd_pcm_t *handle;

    if (snd_pcm_open (&handle, dev_name, SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return 0;

    return handle;
}

static int set_hw_params(snd_pcm_t *handle)
{
    snd_pcm_hw_params_t *params;
    int err;

    if ((err = snd_pcm_hw_params_malloc(&params)) < 0)
        return err;

    if ((err = snd_pcm_hw_params_any(handle, params)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_rate(handle, params, audio_samplerate, 0)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params_set_channels(handle, params, 2)) < 0)
        goto ERR;

    if ((err = snd_pcm_hw_params(handle, params)) < 0)
        goto ERR;

    /*snd_pcm_uframes_t val, val2;
    int dir;
    snd_pcm_hw_params_get_period_size_min(params, &val, &dir);
    snd_pcm_hw_params_get_buffer_size_min(params, &val2);
    printf("%u %u\n", (int)val, (int)val2);*/

ERR:
    snd_pcm_hw_params_free(params);
    return err;
}

static int set_sw_params(snd_pcm_t *handle)
{
    snd_pcm_sw_params_t *params;
    int err;

    if ((err = snd_pcm_sw_params_malloc(&params)) < 0)
        return err;

    if ((err = snd_pcm_sw_params_current(handle, params)) < 0)
        goto ERR;

    if ((err = snd_pcm_sw_params_set_avail_min(handle, params, audio_buffersize)) < 0)
        goto ERR;

    if ((err = snd_pcm_sw_params_set_start_threshold(handle, params, 0)) < 0)
        goto ERR;

    if ((err = snd_pcm_sw_params(handle, params)) < 0)
        goto ERR;

ERR:
    snd_pcm_sw_params_free(params);
    return err;
}

void alsa_thread(void *args)
{
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames_to_deliver;
    sample_t buf[audio_buffersize];

    handle = alsa_open(args);
    if (!handle)
        goto EXIT;

    if (set_hw_params(handle) < 0)
        goto EXIT_CLOSE;

    if (set_sw_params(handle) < 0)
        goto EXIT_CLOSE;

    if (snd_pcm_prepare(handle) < 0)
        goto EXIT_CLOSE;

    alsa_init = 1;
    while (alsa_init) {
        snd_pcm_wait(handle, 1000);
        frames_to_deliver = snd_pcm_avail_update(handle);
        if (frames_to_deliver < 0)
            goto EXIT_CLOSE;

        while (frames_to_deliver >= audio_buffersize) {
            audio_getsamples(&audio, buf, audio_buffersize);
            if (snd_pcm_writei (handle, buf, audio_buffersize) != audio_buffersize)
                goto EXIT_CLOSE;

            frames_to_deliver -= audio_buffersize;
        }
    }

EXIT_CLOSE:
    snd_pcm_close(handle);
EXIT:
    alsa_quit = 1;
}
