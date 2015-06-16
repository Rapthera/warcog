#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

typedef struct {
    void *data;
    int size;
} eventframe_t;

typedef struct {
    eventframe_t frame[256];
} event_t;

void event_clear(event_t *ev, uint8_t frame);
void* event_write(event_t *ev, uint8_t frame, int size);  /* can fail: oom */
void* event_copy(event_t *ev, void *dst, uint8_t start_frame, uint8_t end_frame);
void event_free(event_t *ev);

#endif
