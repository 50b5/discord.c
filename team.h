#ifndef TEAM_H
#define TEAM_H

#include "state.h"

typedef enum discord_team_membership_state {
    TEAM_MEMBERSHIP_INVITED = 1,
    TEAM_MEMBERSHIP_ACCEPTED = 2
} discord_team_membership_state;

typedef struct discord_team_member {
    discord_team_membership_state membership_state;
    list *permissions;
    snowflake team_id;
    const discord_user *user;
} discord_team_member;

typedef struct discord_team {
    discord_state *state;
    json_object *raw_object;

    const char *icon;
    snowflake id;
    list *members;
    const char *name;
    snowflake owner_user_id;
} discord_team;

discord_team *team_init(discord_state *, json_object *);

void team_member_free(void *);
void team_free(void *);

#endif
