#include "net.h"
#include "entity.h"
#include "event.h"
#include "protocol.h"

typedef struct {
    uint16_t id;
    uint8_t firstseen, lastseen;
} clentity_t;


typedef struct {
    uint8_t status, timeout, seq, flags;
    player_t player;

    addr_t addr;

    uint8_t dl_rate, dl_x, delta_frame;
    uint16_t dl_part, dl_missing[12];

    int nents;
    clentity_t *ents;

    event_t ev;
} client_t;

enum {
    cl_none,
    cl_disconnected,
    cl_connected,
    cl_loading,
    cl_loaded,
    cl_loaded_delta,
    cl_loaded_reset,
};

enum {
    cl_timeout,
    cl_disconnect,
    cl_kicked,
};

#define client_timeout 240
#define maxclient 255
client_t client[255];
uint8_t client_id[255], nclient;

client_t* client_new(const addr_t addr);
void client_free(client_t *cl);
void client_join(client_t *cl);
void client_leave(client_t *cl, uint8_t reason);
void client_addentity(client_t *cl, uint16_t id, uint8_t frame);
void client_frame(uint8_t frame);
void client_init(void);

#define client_loop(cl, i) i = 0; while (cl = &client[client_id[i]], ++i <= nclient)
#define cl_id(cl) (int)((cl) - client)
