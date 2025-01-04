#ifndef CHANNEL_H
#define CHANNEL_H

#include "state.h"

typedef enum discord_channel_type {
    CHANNEL_GUILD_TEXT = 0,
    CHANNEL_DM = 1,
    CHANNEL_GUILD_VOICE = 2,
    CHANNEL_GROUP_DM = 3,
    CHANNEL_GUILD_CATEGORY = 4,
    CHANNEL_GUILD_NEWS = 5,
    CHANNEL_GUILD_NEWS_THREAD = 10,
    CHANNEL_GUILD_PUBLIC_THREAD = 11,
    CHANNEL_GUILD_PRIVATE_THREAD = 12,
    CHANNEL_GUILD_STAGE_VOICE = 13,
    CHANNEL_GUILD_DIRECTORY = 14,
    CHANNEL_GUILD_FORUM = 15
} discord_channel_type;

typedef enum discord_channel_video_quality {
    CHANNEL_VIDEO_QUALITY_AUTO = 1,
    CHANNEL_VIDEO_QUALITY_FULL = 2
} discord_channel_video_quality;

typedef enum discord_channel_flags {
    CHANNEL_PINNED = 2
} discord_channel_flags;

typedef struct discord_channel {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    discord_channel_type type;
    snowflake guild_id;
    int position;
    list *permission_overwrites;
    const char *name;
    const char *topic;
    bool nsfw;
    snowflake last_message_id;
    int bitrate;
    int user_limit;
    int rate_limit_per_user;
    list *recipients;
    const char *icon;
    snowflake owner_id;
    snowflake application_id;
    snowflake parent_id;
    const char *last_pin_timestamp;
    const char *rtc_region;
    discord_channel_video_quality video_quality_mode;
    int message_count;
    int member_count;
    //discord_thread_metadata *thread_metadata;
    //discord_thread_member *member;
    int default_auto_archive_duration;
    const char *permissions;
    discord_channel_flags flags;
} discord_channel;

discord_channel *channel_init(discord_state *, json_object *);

/* API calls */
bool channel_send_message(discord_channel *, const discord_message_reply *);

void channel_free(void *);

#endif
