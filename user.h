#ifndef USER_H
#define USER_H

#include "state.h"

#include <json-c/json.h>

typedef struct discord_user_connection {
    const char *id;
    const char *name;
    const char *type;
    bool revoked;
    list *integrations;
    bool verified;
    bool friend_sync;
    bool show_activity;
    int visibility;
} discord_user_connection;

typedef enum discord_user_flags {
    DISCORD_STAFF = 1,
    DISCORD_PARTNER = 2,
    DISCORD_HYPESQUAD = 4,
    DISCORD_BUG_HUNTER_LEVEL_1 = 8,
    DISCORD_MFA_SMS = 16,
    DISCORD_PREMIUM_PROMO_DISMISSED = 32,
    DISCORD_HOUSE_BRAVERY = 64,
    DISCORD_HOUSE_BRILLIANCE = 128,
    DISCORD_HOUSE_BALANCE = 256,
    DISCORD_EARLY_SUPPORTER = 512,
    DISCORD_TEAM_USER = 1024,
    DISCORD_SYSTEM = 4096,
    DISCORD_HAS_UNREAD_URGENT_MESSAGES = 8192,
    DISCORD_BUG_HUNTER_LEVEL_2 = 16384,
    DISCORD_VERIFIED_BOT = 65536,
    DISCORD_VERIFIED_BOT_DEVELOPER = 131072,
    DISCORD_MODERATOR = 262144,
    DISCORD_BOT_HTTP_INTERACTIONS = 524288
} discord_user_flags;

typedef struct discord_user {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    const char *username;
    const char *discriminator;
    const char *avatar;
    bool bot;
    bool system;
    bool mfa_enabled;
    const char *banner;
    int accent_color;
    const char *locale;
    bool verified;
    const char *email;
    int flags;
    int premium_type;
    int public_flags;
} discord_user;

discord_user *user_init(discord_state *, json_object *);

time_t user_get_creation_time(const discord_user *);

void user_free(void *);

#endif
