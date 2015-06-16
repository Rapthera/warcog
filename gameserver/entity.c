#include "entity.h"

static int nfree;
static uint16_t free_id[maxentity];

entity_t* entity_new(void)
{
    return &entity[free_id[--nfree]];
}

void entity_free(entity_t *ent)
{
    free_id[nfree++] = id(ent);
}

void entity_init(void)
{
    int i;
    for (i = 0; i < maxentity; i++)
        free_id[i] = maxentity - (i + 1);

    nfree = maxentity;
}
