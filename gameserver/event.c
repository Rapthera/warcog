#include "event.h"
#include <stdlib.h>
#include <string.h>

void event_clear(event_t *ev, uint8_t frame)
{
    ev->frame[frame].size = 0;
}

void* event_write(event_t *ev, uint8_t frame, int size)
{
    eventframe_t *ef;
    void *data;

    ef = &ev->frame[frame];
    data = realloc(ef->data, ef->size + size);
    if (!data)
        return 0;

    ef->data = data;
    data += ef->size;
    ef->size += size;

    return data;
}

void* event_copy(event_t *ev, void *dst, uint8_t start_frame, uint8_t end_frame)
{
    eventframe_t *ef;
    uint8_t i;

    end_frame++;
    for (i = start_frame; i != end_frame; i++) {
        ef = &ev->frame[i];
        memcpy(dst, ef->data, ef->size);
        dst += ef->size;
    }

    return dst;
}

void event_free(event_t *ev)
{
    int i;

    for (i = 0; i < 256; i++)
        free(ev->frame[i].data);
}
