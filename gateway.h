#ifndef GATEWAY_H
#define GATEWAY_H

#include "state.h"

#include <libwebsockets.h>

typedef struct gateway_receive_buffer gateway_receive_buffer;

typedef enum discord_gateway_opcodes {
    GATEWAY_OP_DISPATCH = 0,
    GATEWAY_OP_HEARTBEAT = 1,
    GATEWAY_OP_IDENTIFY = 2,
    GATEWAY_OP_PRESENCE_UPDATE = 3,
    GATEWAY_OP_VOICE_STATE_UPDATE = 4,
    GATEWAY_OP_RESUME = 6,
    GATEWAY_OP_RECONNECT = 7,
    GATEWAY_OP_REQUEST_GUILD_MEMBERS = 8,
    GATEWAY_OP_INVALID_SESSION = 9,
    GATEWAY_OP_HELLO = 10,
    GATEWAY_OP_HEARTBEAT_ACK = 11,
    GATEWAY_OP_GUILD_SYNC = 12
} discord_gateway_opcodes;

typedef bool (*discord_gateway_event)(void *, const void *);

typedef struct discord_gateway_events {
    const char *name;
    discord_gateway_event event;
} discord_gateway_events;

typedef struct discord_gateway_options {
    bool compress;
    int large_threshold;

    const discord_gateway_events *events;
} discord_gateway_options;

typedef struct discord_gateway {
    discord_state *state;

    /* gateway connection */
    int version;
    bool compress;
    char *endpoint;

    int shards;
    int total_session_starts;
    int remaining_session_starts;
    int reset_after;
    int max_concurrency;

    int large_threshold;
    const discord_gateway_events *events;

    bool running;

    bool connected;
    bool reconnect;
    bool resume;

    char session_id[33];
    int last_sequence;

    unsigned long last_sent;
    int sent_count;

    int heartbeat_interval_us;
    bool awaiting_heartbeat_ack;

    /* websocket */
    struct lws_context *context;
    struct lws *wsi;
    list *queue;
    gateway_receive_buffer *buffer;
} discord_gateway;

discord_gateway *gateway_init(discord_state *, const discord_gateway_options *);

bool gateway_connect(discord_gateway *);
void gateway_disconnect(discord_gateway *);
bool gateway_run_loop(discord_gateway *);

bool gateway_send(discord_gateway *, discord_gateway_opcodes, json_object *);

void gateway_free(discord_gateway *);

#endif
