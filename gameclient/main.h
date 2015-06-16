#include "config.h"

bool do_init(const config_t *cfg);
void do_resize(int width, int height);
void do_cleanup(void);
void do_frame(double delta);
void do_recv(const uint8_t *data, int len);
void do_msg(uint8_t msg, uint8_t v8, uint16_t v16, uint32_t value, void *data);

enum {
    msg_loadparticles,
    msg_loadtext,
    msg_loadmap,
    msg_loadtex,
    msg_loadmdl,
    msg_loaded
};
