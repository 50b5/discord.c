#ifndef EMOJI_H
#define EMOJI_H

#include "state.h"

typedef struct discord_emoji {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    const char *name;
    list *roles;
    const discord_user *user;
    bool require_colons;
    bool managed;
    bool animated;
    bool available;
} discord_emoji;

discord_emoji *emoji_init(discord_state *, json_object *);

void emoji_free(void *);

#endif
