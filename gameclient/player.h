#include <stdint.h>

typedef struct {
    uint8_t status, team, name[16];
} player_t;

typedef struct {
    int count;
    player_t player[255];
} players_t;

players_t players;

void players_clear(players_t *players);
