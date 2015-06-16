#ifndef PROTOCOL_H
#define PROTOCOL_H

enum {
    MSG_BITS = 3,
    CONTROL_BIT_IN = 4,
    CONTROL_BIT_OUT = 8,
    OOP_FLAG = 16,
};

enum {
    flag_info = 1,
    flag_pos = 2,
    flag_anim = 4,
    flag_hp = 8,
    flag_mana = 16,
    flag_ability = 32,
    flag_state = 64,
    flag_spawn = 128,
};

/* server->client */
enum {
    MSG_PART,
    MSG_RESET,
    MSG_DELTA,
    MSG_FRAME
};

enum {
    ev_join,
    ev_leave_timeout,
    ev_leave_timeout_removed,
    ev_leave_disconnect,
    ev_leave_disconnect_removed,
    ev_leave_kicked,
    ev_leave_kicked_removed,
    ev_servermsg,
    ev_end = 0xFF,
};

/* client->server */
enum {
    MSG_MISSING,
    MSG_LOADING,
    MSG_LOADED,
    MSG_CHAT,
    MSG_ORDER,
};

#define partsize 1408

#endif
