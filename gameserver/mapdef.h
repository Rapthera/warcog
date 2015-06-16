#include <stdint.h>

typedef struct {
    uint8_t size;
    uint16_t ndef, nadef, nsdef, neffect;
    uint8_t nparticle, ngs, nattachment, nmdleffect, nstrip, nsoundeff;
    uint16_t ntex, nmodel, nsound;
    uint16_t textlen;
    uint8_t name[16], description[32];
} mapdef_t;
