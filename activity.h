#ifndef ACTIVITY_H
#define ACTIVITY_H

#include "state.h"

typedef enum discord_activity_type {
    ACTIVITY_GAME = 0,
    ACTIVITY_STREAMING = 1,
    ACTIVITY_LISTENING = 2,
    ACTIVITY_WATCHING = 3,
    ACTIVITY_CUSTOM = 4,
    ACTIVITY_COMPETING = 5
} discord_activity_type;

typedef struct discord_activity_timestamps {
    time_t start;
    time_t end;
} discord_activity_timestamps;

typedef struct discord_activity_party {
    const char *id;
    list *size;
} discord_activity_party;

typedef struct discord_activity_assets {
    const char *large_image;
    const char *large_text;
    const char *small_image;
    const char *small_text;
} discord_activity_assets;

typedef struct discord_activity_secrets {
    const char *join;
    const char *spectate;
    const char *match;
} discord_activity_secrets;

typedef enum discord_activity_flags {
    ACTIVITY_FLAG_INSTANCE = 1,
    ACTIVITY_FLAG_JOIN = 2,
    ACTIVITY_FLAG_SPECTATE = 4,
    ACTIVITY_FLAG_JOIN_REQUEST = 8,
    ACTIVITY_FLAG_SYNC = 16,
    ACTIVITY_FLAG_PLAY = 32,
    ACTIVITY_FLAG_PARTY_PRIVACY_FRIENDS = 64,
    ACTIVITY_FLAG_PARTY_PRIVACY_VOICE_CHANNEL = 128,
    ACTIVITY_FLAG_EMBEDDED = 256
} discord_activity_flags;

typedef struct discord_activity_button {
    const char *label;
    const char *url;
} discord_activity_button;

typedef struct discord_activity {
    discord_state *state;
    json_object *raw_object;

    const char *name;
    discord_activity_type type;
    const char *url;
    time_t created_at;
    discord_activity_timestamps *timestamps;
    snowflake application_id;
    const char *details;
    const char *status;
    const discord_emoji *emoji;
    discord_activity_party *party;
    discord_activity_assets *assets;
    discord_activity_secrets *secrets;
    bool instance;
    int flags;
    list *buttons;
} discord_activity;

discord_activity_type activity_type_from_string(const char *);

discord_activity *activity_init(discord_state *, json_object *);

json_object *activity_to_json(const discord_activity *);

bool activity_set_name(discord_activity *, const char *);
bool activity_set_type(discord_activity *, discord_activity_type);
bool activity_set_url(discord_activity *, const char *);

void activity_free(void *);

#endif
