#ifndef ROLE_H
#define ROLE_H

#include "state.h"

typedef struct discord_role_tags {
    snowflake bot_id;
    snowflake integration_id;
    bool premium_subscriber; /* null in docs? ignore it for now */
} discord_role_tags;

typedef struct discord_role {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    const char *name;
    int color;
    bool hoist;
    const char *icon;
    const char *unicode_emoji;
    int position;
    const char *permissions;
    bool managed;
    bool mentionable;
    discord_role_tags *tags;
} discord_role;

discord_role *role_init(discord_state *, json_object *);

void role_free(void *);

#endif
