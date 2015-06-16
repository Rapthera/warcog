#include <stdbool.h>

extern volatile bool alsa_init, alsa_quit;

void alsa_thread(void *args);
