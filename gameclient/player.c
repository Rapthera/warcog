#include "player.h"

void players_clear(players_t *players)
{
    int i;

    players->count = 0;
    for (i = 0; i < 255; i++)
        players->player[i].status = 0;
}
