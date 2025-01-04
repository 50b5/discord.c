#ifndef MEMBER_H
#define MEMBER_H

#include "state.h"

#include <json-c/json.h>

typedef struct discord_member {
    discord_state *state;
    json_object *raw_object;

    const discord_user *user;
    const char *nick;
    const char *avatar;
    list *roles;
    const char *joined_at;
    const char *premium_since;
    bool deaf;
    bool mute;
    bool pending;
    const char *permissions;
    const char *communication_disabled_until;
} discord_member;

discord_member *member_init(discord_state *, json_object *);

void member_free(void *);

#endif
