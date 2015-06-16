/* mostly modified OpenAL Soft code */
//#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#define M_2PI (M_PI * 2.0)

#include "hrtf_data.h"
#include "hrtf.h"
#include "../math.h"

uint32_t minu(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

float minf(float a, float b)
{
    return (a < b) ? a : b;
}

float maxf(float a, float b)
{
    return (a > b) ? a : b;
}

float clampf(float val, float min, float max)
{
    return minf(max, maxf(min, val));
}

float lerp(float a, float b, float k)
{
    return a + (b - a) * k;
}

uint32_t f2u(float a)
{
    return a;
}

int f2i(float a)
{
    return a;
}

#define alignas(x)
#define GAIN_SILENCE_THRESHOLD  (0.00001f)

/* Current data set limits defined by the makehrtf utility. */
#define MIN_IR_SIZE                  (8)
#define MAX_IR_SIZE                  (128)
#define MOD_IR_SIZE                  (8)

#define MIN_EV_COUNT                 (5)
#define MAX_EV_COUNT                 (128)

#define MIN_AZ_COUNT                 (1)
#define MAX_AZ_COUNT                 (128)

/* First value for pass-through coefficients (remaining are 0), used for omni-
 * directional sounds. */
static const float PassthruCoeff = 32767.0f * 0.707106781187f/*sqrt(0.5)*/;

/* Calculate the elevation indices given the polar elevation in radians.
 * This will return two indices between 0 and (evcount - 1) and an
 * interpolation factor between 0.0 and 1.0.
 */
static void CalcEvIndices(uint32_t evcount, float ev, uint32_t *evidx, float *evmu)
{
    ev = (M_PI_2 + ev) * (evcount-1) / M_PI;
    evidx[0] = f2u(ev);
    evidx[1] = minu(evidx[0] + 1, evcount-1);
    *evmu = ev - evidx[0];
}

/* Calculate the azimuth indices given the polar azimuth in radians.  This
 * will return two indices between 0 and (azcount - 1) and an interpolation
 * factor between 0.0 and 1.0.
 */
static void CalcAzIndices(uint32_t azcount, float az, uint32_t *azidx, float *azmu)
{
    az = (M_2PI + az) * azcount / M_2PI;
    azidx[0] = f2u(az) % azcount;
    azidx[1] = (azidx[0] + 1) % azcount;
    *azmu = az - floorf(az);
}

/* Calculates static HRIR coefficients and delays for the given polar
 * elevation and azimuth in radians.  Linear interpolation is used to
 * increase the apparent resolution of the HRIR data set.  The coefficients
 * are also normalized and attenuated by the specified gain.
 */
void hrtf_lerped(hrtf_params_t *res, float elevation, float azimuth, float dirfact, float gain)
{
    uint32_t evidx[2], lidx[4], ridx[4];
    float mu[3], blend[4];
    uint32_t i;

    /* Claculate elevation indices and interpolation factor. */
    CalcEvIndices(hrtf_evcount, elevation, evidx, &mu[2]);

    for(i = 0;i < 2;i++)
    {
        uint32_t azcount = hrtf_azcount[evidx[i]];
        uint32_t evoffset = hrtf_evoffset[evidx[i]];
        uint32_t azidx[2];

        /* Calculate azimuth indices and interpolation factor for this elevation. */
        CalcAzIndices(azcount, azimuth, azidx, &mu[i]);

        /* Calculate a set of linear HRIR indices for left and right channels. */
        lidx[i*2 + 0] = evoffset + azidx[0];
        lidx[i*2 + 1] = evoffset + azidx[1];
        ridx[i*2 + 0] = evoffset + ((azcount-azidx[0]) % azcount);
        ridx[i*2 + 1] = evoffset + ((azcount-azidx[1]) % azcount);
    }

    /* Calculate 4 blending weights for 2D bilinear interpolation. */
    blend[0] = (1.0f-mu[0]) * (1.0f-mu[2]);
    blend[1] = (     mu[0]) * (1.0f-mu[2]);
    blend[2] = (1.0f-mu[1]) * (     mu[2]);
    blend[3] = (     mu[1]) * (     mu[2]);

    /* Calculate the HRIR delays using linear interpolation. */
    res->delay[0] = f2u((hrtf_delays[lidx[0]]*blend[0] + hrtf_delays[lidx[1]]*blend[1] +
                         hrtf_delays[lidx[2]]*blend[2] + hrtf_delays[lidx[3]]*blend[3]) *
                        dirfact + 0.5f) << HRTFDELAY_BITS;
    res->delay[1] = f2u((hrtf_delays[ridx[0]]*blend[0] + hrtf_delays[ridx[1]]*blend[1] +
                         hrtf_delays[ridx[2]]*blend[2] + hrtf_delays[ridx[3]]*blend[3]) *
                        dirfact + 0.5f) << HRTFDELAY_BITS;

    /* Calculate the sample offsets for the HRIR indices. */
    lidx[0] *= hrtf_irsize;
    lidx[1] *= hrtf_irsize;
    lidx[2] *= hrtf_irsize;
    lidx[3] *= hrtf_irsize;
    ridx[0] *= hrtf_irsize;
    ridx[1] *= hrtf_irsize;
    ridx[2] *= hrtf_irsize;
    ridx[3] *= hrtf_irsize;

    /* Calculate the normalized and attenuated HRIR coefficients using linear
     * interpolation when there is enough gain to warrant it.  Zero the
     * coefficients if gain is too low.
     */
    if(gain > 0.0001f)
    {
        float c;

        i = 0;
        c = (hrtf_coeffs[lidx[0]+i]*blend[0] + hrtf_coeffs[lidx[1]+i]*blend[1] +
             hrtf_coeffs[lidx[2]+i]*blend[2] + hrtf_coeffs[lidx[3]+i]*blend[3]);
        res->coeffs[i][0] = lerp(PassthruCoeff, c, dirfact) * gain * (1.0f/32767.0f);
        c = (hrtf_coeffs[ridx[0]+i]*blend[0] + hrtf_coeffs[ridx[1]+i]*blend[1] +
             hrtf_coeffs[ridx[2]+i]*blend[2] + hrtf_coeffs[ridx[3]+i]*blend[3]);
        res->coeffs[i][1] = lerp(PassthruCoeff, c, dirfact) * gain * (1.0f/32767.0f);

        for(i = 1;i < hrtf_irsize;i++)
        {
            c = (hrtf_coeffs[lidx[0]+i]*blend[0] + hrtf_coeffs[lidx[1]+i]*blend[1] +
                 hrtf_coeffs[lidx[2]+i]*blend[2] + hrtf_coeffs[lidx[3]+i]*blend[3]);
            res->coeffs[i][0] = lerp(0.0f, c, dirfact) * gain * (1.0f/32767.0f);
            c = (hrtf_coeffs[ridx[0]+i]*blend[0] + hrtf_coeffs[ridx[1]+i]*blend[1] +
                 hrtf_coeffs[ridx[2]+i]*blend[2] + hrtf_coeffs[ridx[3]+i]*blend[3]);
            res->coeffs[i][1] = lerp(0.0f, c, dirfact) * gain * (1.0f/32767.0f);
        }
    }
    else
    {
        for(i = 0;i < hrtf_irsize;i++)
        {
            res->coeffs[i][0] = 0.0f;
            res->coeffs[i][1] = 0.0f;
        }
    }
}

/* Calculates the moving HRIR target coefficients, target delays, and
 * stepping values for the given polar elevation and azimuth in radians.
 * Linear interpolation is used to increase the apparent resolution of the
 * HRIR data set.  The coefficients are also normalized and attenuated by the
 * specified gain.  Stepping resolution and count is determined using the
 * given delta factor between 0.0 and 1.0.
 */
uint32_t hrtf_moving(hrtf_params_t *res, float elevation, float azimuth, float dirfact, float gain,
                     float delta, int counter)
{
    uint32_t evidx[2], lidx[4], ridx[4];
    float mu[3], blend[4];
    float left, right;
    float steps;
    uint32_t i;

    /* Claculate elevation indices and interpolation factor. */
    CalcEvIndices(hrtf_evcount, elevation, evidx, &mu[2]);

    for(i = 0;i < 2;i++)
    {
        uint32_t azcount = hrtf_azcount[evidx[i]];
        uint32_t evoffset = hrtf_evoffset[evidx[i]];
        uint32_t azidx[2];

        /* Calculate azimuth indices and interpolation factor for this elevation. */
        CalcAzIndices(azcount, azimuth, azidx, &mu[i]);

        /* Calculate a set of linear HRIR indices for left and right channels. */
        lidx[i*2 + 0] = evoffset + azidx[0];
        lidx[i*2 + 1] = evoffset + azidx[1];
        ridx[i*2 + 0] = evoffset + ((azcount-azidx[0]) % azcount);
        ridx[i*2 + 1] = evoffset + ((azcount-azidx[1]) % azcount);
    }

    // Calculate the stepping parameters.
    steps = maxf(floorf(delta*hrtf_samplerate + 0.5f), 1.0f);
    delta = 1.0f / steps;

    /* Calculate 4 blending weights for 2D bilinear interpolation. */
    blend[0] = (1.0f-mu[0]) * (1.0f-mu[2]);
    blend[1] = (     mu[0]) * (1.0f-mu[2]);
    blend[2] = (1.0f-mu[1]) * (     mu[2]);
    blend[3] = (     mu[1]) * (     mu[2]);

    /* Calculate the HRIR delays using linear interpolation.  Then calculate
     * the delay stepping values using the target and previous running
     * delays.
     */
    left = (float)(res->delay[0] - (res->delay_step[0] * counter));
    right = (float)(res->delay[1] - (res->delay_step[1] * counter));

    res->delay[0] = f2u((hrtf_delays[lidx[0]]*blend[0] + hrtf_delays[lidx[1]]*blend[1] +
                         hrtf_delays[lidx[2]]*blend[2] + hrtf_delays[lidx[3]]*blend[3]) *
                        dirfact + 0.5f) << HRTFDELAY_BITS;
    res->delay[1] = f2u((hrtf_delays[ridx[0]]*blend[0] + hrtf_delays[ridx[1]]*blend[1] +
                         hrtf_delays[ridx[2]]*blend[2] + hrtf_delays[ridx[3]]*blend[3]) *
                        dirfact + 0.5f) << HRTFDELAY_BITS;

    res->delay_step[0] = f2i(delta * (res->delay[0] - left));
    res->delay_step[1] = f2i(delta * (res->delay[1] - right));

    /* Calculate the sample offsets for the HRIR indices. */
    lidx[0] *= hrtf_irsize;
    lidx[1] *= hrtf_irsize;
    lidx[2] *= hrtf_irsize;
    lidx[3] *= hrtf_irsize;
    ridx[0] *= hrtf_irsize;
    ridx[1] *= hrtf_irsize;
    ridx[2] *= hrtf_irsize;
    ridx[3] *= hrtf_irsize;

    /* Calculate the normalized and attenuated target HRIR coefficients using
     * linear interpolation when there is enough gain to warrant it.  Zero
     * the target coefficients if gain is too low.  Then calculate the
     * coefficient stepping values using the target and previous running
     * coefficients.
     */
    if(gain > 0.0001f)
    {
        float c;

        i = 0;
        left = res->coeffs[i][0] - (res->coeff_step[i][0] * counter);
        right = res->coeffs[i][1] - (res->coeff_step[i][1] * counter);

        c = (hrtf_coeffs[lidx[0]+i]*blend[0] + hrtf_coeffs[lidx[1]+i]*blend[1] +
             hrtf_coeffs[lidx[2]+i]*blend[2] + hrtf_coeffs[lidx[3]+i]*blend[3]);
        res->coeffs[i][0] = lerp(PassthruCoeff, c, dirfact) * gain * (1.0f/32767.0f);
        c = (hrtf_coeffs[ridx[0]+i]*blend[0] + hrtf_coeffs[ridx[1]+i]*blend[1] +
             hrtf_coeffs[ridx[2]+i]*blend[2] + hrtf_coeffs[ridx[3]+i]*blend[3]);
        res->coeffs[i][1] = lerp(PassthruCoeff, c, dirfact) * gain * (1.0f/32767.0f);

        res->coeff_step[i][0] = delta * (res->coeffs[i][0] - left);
        res->coeff_step[i][1] = delta * (res->coeffs[i][1] - right);

        for(i = 1;i < hrtf_irsize;i++)
        {
            left = res->coeffs[i][0] - (res->coeff_step[i][0] * counter);
            right = res->coeffs[i][1] - (res->coeff_step[i][1] * counter);

            c = (hrtf_coeffs[lidx[0]+i]*blend[0] + hrtf_coeffs[lidx[1]+i]*blend[1] +
                 hrtf_coeffs[lidx[2]+i]*blend[2] + hrtf_coeffs[lidx[3]+i]*blend[3]);
            res->coeffs[i][0] = lerp(0.0f, c, dirfact) * gain * (1.0f/32767.0f);
            c = (hrtf_coeffs[ridx[0]+i]*blend[0] + hrtf_coeffs[ridx[1]+i]*blend[1] +
                 hrtf_coeffs[ridx[2]+i]*blend[2] + hrtf_coeffs[ridx[3]+i]*blend[3]);
            res->coeffs[i][1] = lerp(0.0f, c, dirfact) * gain * (1.0f/32767.0f);

            res->coeff_step[i][0] = delta * (res->coeffs[i][0] - left);
            res->coeff_step[i][1] = delta * (res->coeffs[i][1] - right);
        }
    }
    else
    {
        for(i = 0;i < hrtf_irsize;i++)
        {
            left = res->coeffs[i][0] - (res->coeff_step[i][0] * counter);
            right = res->coeffs[i][1] - (res->coeff_step[i][1] * counter);

            res->coeffs[i][0] = 0.0f;
            res->coeffs[i][1] = 0.0f;

            res->coeff_step[i][0] = delta * -left;
            res->coeff_step[i][1] = delta * -right;
        }
    }

    /* The stepping count is the number of samples necessary for the HRIR to
     * complete its transition.  The mixer will only apply stepping for this
     * many samples.
     */
    return f2u(steps);
}


/* Calculates HRTF coefficients for B-Format channels (only up to first-order). */
void GetBFormatHrtfCoeffs(const uint32_t num_chans, float (**coeffs_list)[2], uint32_t **delay_list)
{
    uint32_t elev_idx, azi_idx;
    float scale;
    uint32_t i, c;

    assert(num_chans <= 4);

    for(c = 0;c < num_chans;c++)
    {
        float (*coeffs)[2] = coeffs_list[c];
        uint32_t *delay = delay_list[c];

        for(i = 0;i < hrtf_irsize;i++)
        {
            coeffs[i][0] = 0.0f;
            coeffs[i][1] = 0.0f;
        }
        delay[0] = 0;
        delay[1] = 0;
    }

    /* NOTE: HRTF coefficients are generated by combining all the HRIRs in the
     * dataset, with each entry scaled according to how much it contributes to
     * the given B-Format channel based on its direction (including negative
     * contributions!).
     */
    scale = 0.0f;
    for(elev_idx = 0;elev_idx < hrtf_evcount;elev_idx++)
    {
        float elev = (float)elev_idx/(float)(hrtf_evcount-1)*M_PI - M_PI_2;
        uint32_t evoffset = hrtf_evoffset[elev_idx];
        uint32_t azcount = hrtf_azcount[elev_idx];

        scale += (float)azcount;

        for(azi_idx = 0;azi_idx < azcount;azi_idx++)
        {
            uint32_t lidx, ridx;
            float ambi_coeffs[4];
            float az, gain;
            float x, y, z;

            lidx = evoffset + azi_idx;
            ridx = evoffset + ((azcount-azi_idx) % azcount);

            az = (float)azi_idx / (float)azcount * M_2PI;
            if(az > M_PI) az -= M_2PI;

            x = cosf(-az) * cosf(elev);
            y = sinf(-az) * cosf(elev);
            z = sinf(elev);

            ambi_coeffs[0] = 1.4142f;
            ambi_coeffs[1] = x; /* X */
            ambi_coeffs[2] = y; /* Y */
            ambi_coeffs[3] = z; /* Z */

            for(c = 0;c < num_chans;c++)
            {
                float (*coeffs)[2] = coeffs_list[c];
                uint32_t *delay = delay_list[c];

                /* NOTE: Always include the total delay average since the
                 * channels need to have matching delays. */
                delay[0] += hrtf_delays[lidx];
                delay[1] += hrtf_delays[ridx];

                gain = ambi_coeffs[c];
                if(!(fabsf(gain) > GAIN_SILENCE_THRESHOLD))
                    continue;

                for(i = 0;i < hrtf_irsize;i++)
                {
                    coeffs[i][0] += hrtf_coeffs[lidx*hrtf_irsize + i]*(1.0f/32767.0f) * gain;
                    coeffs[i][1] += hrtf_coeffs[ridx*hrtf_irsize + i]*(1.0f/32767.0f) * gain;
                }
            }
        }
    }

    scale = 1.0f/scale;

    for(c = 0;c < num_chans;c++)
    {
        float (*coeffs)[2] = coeffs_list[c];
        uint32_t *delay = delay_list[c];

        for(i = 0;i < hrtf_irsize;i++)
        {
            coeffs[i][0] *= scale;
            coeffs[i][1] *= scale;
        }
        delay[0] = minu((uint32_t)((float)delay[0] * scale), HRTF_HISTORY_LENGTH-1);
        delay[0] <<= HRTFDELAY_BITS;
        delay[1] = minu((uint32_t)((float)delay[1] * scale), HRTF_HISTORY_LENGTH-1);
        delay[1] <<= HRTFDELAY_BITS;
    }
}

static void SetupCoeffs(float (*OutCoeffs)[2], const hrtf_params_t *hrtfparams,
                        uint32_t IrSize, uint32_t Counter)
{
    uint32_t c;
    for(c = 0;c < IrSize;c++)
    {
        OutCoeffs[c][0] = hrtfparams->coeffs[c][0] - (hrtfparams->coeff_step[c][0]*Counter);
        OutCoeffs[c][1] = hrtfparams->coeffs[c][1] - (hrtfparams->coeff_step[c][1]*Counter);
    }
}

static void ApplyCoeffsStep(uint32_t Offset, float (* Values)[2],
                                   const uint32_t IrSize,
                                   float (* Coeffs)[2],
                                   const float (* CoeffStep)[2],
                                   float left, float right)
{
    uint32_t c;
    for(c = 0;c < IrSize;c++)
    {
        const uint32_t off = (Offset+c)&HRIR_MASK;
        Values[off][0] += Coeffs[c][0] * left;
        Values[off][1] += Coeffs[c][1] * right;
        Coeffs[c][0] += CoeffStep[c][0];
        Coeffs[c][1] += CoeffStep[c][1];
    }
}

static void ApplyCoeffs(uint32_t Offset, float (* Values)[2],
                               const uint32_t IrSize,
                               float (* Coeffs)[2],
                               float left, float right)
{
    uint32_t c;
    for(c = 0;c < IrSize;c++)
    {
        const uint32_t off = (Offset+c)&HRIR_MASK;
        Values[off][0] += Coeffs[c][0] * left;
        Values[off][1] += Coeffs[c][1] * right;
    }
}

void hrtf_mix(sample_t *buf, const float *data, uint32_t Counter, uint32_t Offset, uint32_t OutPos,
             const hrtf_params_t *hrtfparams, hrtf_state_t *hrtfstate, uint32_t BufferSize)
{
    float Coeffs[HRIR_LENGTH][2];
    uint32_t Delay[2];
    float left, right;
    uint32_t pos;

    SetupCoeffs(Coeffs, hrtfparams, hrtf_irsize, Counter);
    Delay[0] = hrtfparams->delay[0] - (hrtfparams->delay_step[0]*Counter);
    Delay[1] = hrtfparams->delay[1] - (hrtfparams->delay_step[1]*Counter);

    pos = 0;
    for(;pos < BufferSize && pos < Counter;pos++)
    {
        hrtfstate->history[Offset&HRTF_HISTORY_MASK] = data[pos];
        left  = lerp(hrtfstate->history[(Offset-(Delay[0]>>HRTFDELAY_BITS))&HRTF_HISTORY_MASK],
                     hrtfstate->history[(Offset-(Delay[0]>>HRTFDELAY_BITS)-1)&HRTF_HISTORY_MASK],
                     (Delay[0]&HRTFDELAY_MASK)*(1.0f/HRTFDELAY_FRACONE));
        right = lerp(hrtfstate->history[(Offset-(Delay[1]>>HRTFDELAY_BITS))&HRTF_HISTORY_MASK],
                     hrtfstate->history[(Offset-(Delay[1]>>HRTFDELAY_BITS)-1)&HRTF_HISTORY_MASK],
                     (Delay[1]&HRTFDELAY_MASK)*(1.0f/HRTFDELAY_FRACONE));

        Delay[0] += hrtfparams->delay_step[0];
        Delay[1] += hrtfparams->delay_step[1];

        hrtfstate->values[(Offset+hrtf_irsize)&HRIR_MASK][0] = 0.0f;
        hrtfstate->values[(Offset+hrtf_irsize)&HRIR_MASK][1] = 0.0f;
        Offset++;

        ApplyCoeffsStep(Offset, hrtfstate->values, hrtf_irsize, Coeffs, hrtfparams->coeff_step, left, right);
        buf[OutPos].left += hrtfstate->values[Offset&HRIR_MASK][0];
        buf[OutPos].right += hrtfstate->values[Offset&HRIR_MASK][1];
        OutPos++;
    }

    Delay[0] >>= HRTFDELAY_BITS;
    Delay[1] >>= HRTFDELAY_BITS;
    for(;pos < BufferSize;pos++)
    {
        hrtfstate->history[Offset&HRTF_HISTORY_MASK] = data[pos];
        left = hrtfstate->history[(Offset-Delay[0])&HRTF_HISTORY_MASK];
        right = hrtfstate->history[(Offset-Delay[1])&HRTF_HISTORY_MASK];

        hrtfstate->values[(Offset+hrtf_irsize)&HRIR_MASK][0] = 0.0f;
        hrtfstate->values[(Offset+hrtf_irsize)&HRIR_MASK][1] = 0.0f;
        Offset++;

        ApplyCoeffs(Offset, hrtfstate->values, hrtf_irsize, Coeffs, left, right);
        buf[OutPos].left += hrtfstate->values[Offset&HRIR_MASK][0];
        buf[OutPos].right += hrtfstate->values[Offset&HRIR_MASK][1];
        OutPos++;
    }
}

float hrtf_calcfadetime(float oldGain, float newGain, const vec3 *olddir, const vec3 *newdir)
{
    float gainChange, angleChange, change;

    /* Calculate the normalized dB gain change. */
    newGain = maxf(newGain, 0.0001f);
    oldGain = maxf(oldGain, 0.0001f);
    gainChange = fabsf(log10f(newGain / oldGain) / log10f(0.0001f));

    /* Calculate the angle change only when there is enough gain to notice it. */
    angleChange = 0.0f;
    if(gainChange > 0.0001f || newGain > 0.0001f)
    {
        /* No angle change when the directions are equal or degenerate (when
         * both have zero length).
         */
        if(newdir->v[0] != olddir->v[0] || newdir->v[1] != olddir->v[1] || newdir->v[2] != olddir->v[2])
        {
            float dotp = dot3(*olddir, *newdir);
            angleChange = acosf(clampf(dotp, -1.0f, 1.0f)) / M_PI;
        }
    }

    /* Use the largest of the two changes, and apply a significance shaping
     * function to it. The result is then scaled to cover a 15ms transition
     * range.
     */
    change = maxf(angleChange * 25.0f, gainChange) * 2.0f;
    return minf(change, 1.0f) * 0.015f;
}


