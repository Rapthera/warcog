#include <stdint.h>

#define hrtf_samplerate 44100
#define hrtf_irsize 32
#define hrtf_evcount 19

extern const uint8_t hrtf_azcount[];
extern const uint16_t hrtf_evoffset[];
extern const int16_t hrtf_coeffs[];
extern const uint8_t hrtf_delays[];
