#ifndef GUILD_H
#define GUILD_H

#include "state.h"

#include "scheduled_event.h"

typedef enum discord_guild_verification_level {
    GUILD_VERIFICATION_NONE = 0,
    GUILD_VERIFICATION_LOW = 1,
    GUILD_VERIFICATION_MEDIUM = 2,
    GUILD_VERIFICATION_HIGH = 3,
    GUILD_VERIFICATION_VERY_HIGH = 4
} discord_guild_verification_level;

typedef enum discord_guild_notification_level {
    GUILD_NOTIFICATION_ALL_MESSAGES = 0,
    GUILD_NOTIFICATION_ONLY_MENTIONS = 1
} discord_guild_notification_level;

typedef enum discord_guild_filter_level {
    GUILD_FILTER_DISABLED = 0,
    GUILD_FILTER_MEMBERS_WITHOUT_ROLES = 1,
    GUILD_FILTER_ALL_MEMBERS = 2
} discord_guild_filter_level;

typedef enum discord_guild_mfa_level {
    GUILD_MFA_NONE = 0,
    GUILD_MFA_ELEVATED = 1
} discord_guild_mfa_level;

typedef enum discord_guild_system_channel_flags {
    GUILD_SUPPRESS_JOIN_NOTIFICATIONS = 1,
    GUILD_SUPPRESS_PREMIUM_SUBSCRIPTIONS = 2,
    GUILD_SUPPRESS_GUILD_REMINDER_NOTIFICATIONS = 4,
    GUILD_SUPPRESS_JOIN_NOTIFICATION_REPLIES = 8
} discord_guild_system_channel_flags;

typedef enum discord_guild_premium_tier {
    GUILD_PREMIUM_NONE = 0,
    GUILD_PREMIUM_TIER_1 = 1,
    GUILD_PREMIUM_TIER_2 = 2,
    GUILD_PREMIUM_TIER_3 = 3
} discord_guild_premium_tier;

typedef enum discord_guild_nsfw_level {
    GUILD_NSFW_DEFAULT = 0,
    GUILD_NSFW_EXPLICIT = 1,
    GUILD_NSFW_SAFE = 2,
    GUILD_NSFW_AGE_RESTRICTED = 3
} discord_guild_nsfw_level;

typedef struct discord_guild {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    const char *name;
    const char *icon;
    const char *icon_hash;
    const char *splash;
    const char *discovery_splash;
    bool owner;
    snowflake owner_id;
    const char *permissions;
    snowflake afk_channel_id;
    int afk_timeout;
    bool widget_enabled;
    snowflake widget_channel_id;
    discord_guild_verification_level verification_level;
    discord_guild_notification_level default_message_notifications;
    discord_guild_filter_level explicit_content_filter;
    map *roles;
    list *emojis;
    list *features;
    discord_guild_mfa_level mfa_level;
    snowflake application_id;
    snowflake system_channel_id;
    discord_guild_system_channel_flags system_channel_flags;
    snowflake rules_channel_id;
    const char *joined_at;
    bool large;
    bool unavailable;
    int member_count;
    list *voice_states;
    map *members;
    map *channels;
    map *threads;
    int max_presences;
    int max_members;
    const char *vanity_url_code;
    const char *description;
    const char *banner;
    discord_guild_premium_tier premium_tier;
    int premium_subscription_count;
    const char *preferred_locale;
    snowflake public_updates_channel_id;
    int max_video_channel_users;
    int approximate_member_count;
    int approximate_presence_count;
    //discord_welcome_screen *welcome_screen;
    discord_guild_nsfw_level nsfw_level;
    list *stage_instances;
    list *stickers;
    list *guild_scheduled_events;
    bool premium_progress_bar_enabled;
} discord_guild;

discord_guild *guild_init(discord_state *, json_object *);

void guild_free(void *);

#endif
