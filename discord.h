#ifndef DISCORD_H
#define DISCORD_H

#include "gateway.h"
#include "state.h"

typedef struct discord_options {
    const logctx *log;

    discord_gateway_intents intent;
    const discord_presence *presence;

    /* passthrough state options */
    size_t max_messages;

    /* passthrough gateway options */
    bool compress;
    int large_threshold;
    const discord_gateway_events *events;
} discord_options;

typedef struct discord {
    discord_state *state;
    discord_gateway *gateway;

    discord_application *application;
    const discord_user *user;
} discord;

discord *discord_init(const char *, const discord_options *);

bool discord_connect_gateway(discord *);
void discord_disconnect_gateway(discord *);

bool discord_set_presence(discord *, const discord_presence *);
bool discord_modify_presence(discord *, const time_t *, const list *, const char *, const bool *);

const discord_user *discord_get_user(discord *, snowflake, bool);

bool discord_send_message(discord *, snowflake, const discord_message_reply *);

void discord_free(discord *);

#endif
