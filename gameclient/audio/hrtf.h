#include <stdint.h>
#include "../math.h"
#include "sample.h"

#define HRIR_BITS        (7)
#define HRIR_LENGTH      (1<<HRIR_BITS)
#define HRIR_MASK        (HRIR_LENGTH-1)
#define HRTFDELAY_BITS    (20)
#define HRTFDELAY_FRACONE (1<<HRTFDELAY_BITS)
#define HRTFDELAY_MASK    (HRTFDELAY_FRACONE-1)
#define HRTF_HISTORY_BITS   (6)
#define HRTF_HISTORY_LENGTH (1<<HRTF_HISTORY_BITS)
#define HRTF_HISTORY_MASK   (HRTF_HISTORY_LENGTH-1)

typedef struct {
    float coeffs[HRIR_LENGTH][2];
    float coeff_step[HRIR_LENGTH][2];
    uint32_t delay[2];
    int delay_step[2];
} hrtf_params_t;

typedef struct {
    float history[HRTF_HISTORY_LENGTH];
    float values[HRIR_LENGTH][2];
} hrtf_state_t;

float hrtf_calcfadetime(float oldGain, float newGain, const vec3 *olddir, const vec3 *newdir);

void hrtf_lerped(hrtf_params_t *res, float elevation, float azimuth, float dirfact, float gain);
uint32_t hrtf_moving(hrtf_params_t *res, float elevation, float azimuth, float dirfact, float gain,
                     float delta, int counter);

void hrtf_mix(sample_t *buf, const float *data, uint32_t Counter, uint32_t Offset, uint32_t OutPos,
             const hrtf_params_t *hrtfparams, hrtf_state_t *hrtfstate, uint32_t BufferSize);
