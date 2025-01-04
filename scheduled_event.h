#ifndef SCHEDULED_EVENT_H
#define SCHEDULED_EVENT_H

#include "state.h"

typedef enum discord_scheduled_event_privacy_level {
    SCHEDULED_EVENT_GUILD_ONLY = 2
} discord_scheduled_event_privacy_level;

typedef enum discord_scheduled_event_status {
    SCHEDULED_EVENT_STATUS_SCHEDULED = 1,
    SCHEDULED_EVENT_STATUS_ACTIVE = 2,
    SCHEDULED_EVENT_STATUS_COMPLETED = 3,
    SCHEDULED_EVENT_STATUS_CANCELED = 4
} discord_scheduled_event_status;

typedef enum discord_scheduled_event_entity_type {
    SCHEDULED_EVENT_ENTITY_STAGE_INSTANCE = 1,
    SCHEDULED_EVENT_ENTITY_VOICE = 2,
    SCHEDULED_EVENT_ENTITY_EXTERNAL = 3
} discord_scheduled_event_entity_type;

typedef struct discord_scheduled_event_entity_metadata {
    const char *location;
} discord_scheduled_event_entity_metadata;

typedef struct discord_guild_scheduled_event {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    snowflake guild_id;
    snowflake channel_id;
    snowflake creator_id;
    const char *name;
    const char *description;
    const char *scheduled_start_time;
    const char *scheduled_end_time;
    discord_scheduled_event_privacy_level privacy_level;
    discord_scheduled_event_status status;
    discord_scheduled_event_entity_type entity_type;
    snowflake entity_id;
    discord_scheduled_event_entity_metadata *entity_metadata;
    const discord_user *creator;
    int user_count;
    const char *image;
} discord_scheduled_event;

discord_scheduled_event scheduled_event_init(discord_state *, json_object *);

void scheduled_event_free(void *);

#endif
