#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool active;
    int count;
    uint16_t ent[256];

    int x, y, x2, y2;
} selection_t;

typedef struct {
    bool up, down, left, right, middle, shift;
    int mx, my;

    uint8_t target;
    uint16_t ability, target_ent;
    uint32_t tx, ty;

    float cx, cy, zoom, theta;
    double voice_cd, pradius;
    uint32_t px, py;

    selection_t sel;
} input_t;

input_t input;

void input_keydown(input_t *in, uint32_t keycode, uint32_t unicode);
void input_keyup(input_t *in, uint32_t keycode);

void input_mmove(input_t *in, int x, int y, uint8_t id);
void input_mdown(input_t *in, int x, int y, uint8_t id, uint8_t button);
void input_mup(input_t *in, int x, int y, uint8_t id, int8_t button);
void input_mleave(input_t *in, uint8_t id);
void input_mwheel(input_t *in, double delta);

void input_frame(input_t *in, double delta);
void input_validsel(input_t *in);
