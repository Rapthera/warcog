#include <stdint.h>
#include <stdbool.h>
#include "net.h"

/* called by platform specific */
void on_frame(void);
void on_recv(const uint8_t *data, int len, const addr_t addr);
bool on_init(void);
