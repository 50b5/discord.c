#ifndef MESSAGE_H
#define MESSAGE_H

#include "state.h"

typedef struct discord_message_reference {
    snowflake message_id;
    snowflake channel_id;
    snowflake guild_id;
    bool fail_if_not_exists;
} discord_message_reference;

typedef struct discord_message_reply_allowed_mentions {
    const list *parse;
    const list *roles;
    const list *users;
    bool replied_user;
} discord_message_reply_allowed_mentions;

typedef struct discord_message_reply {
    const char *content;
    bool tts;
    const discord_embed *embed;
    const list *embeds;
    const discord_message_reply_allowed_mentions *allowed_mentions;
    const discord_message_reference *message_reference;
    const list *components;
    const list *sticker_ids;
    json_object *payload_json;
    const list *attachments;
    int flags;
} discord_message_reply;

typedef enum discord_message_type {
    MESSAGE_DEFAULT = 0,
    MESSAGE_RECIPIENT_ADD = 1,
    MESSAGE_RECIPIENT_REMOVE = 2,
    MESSAGE_CALL = 3,
    MESSAGE_CHANNEL_NAME_CHANGE = 4,
    MESSAGE_CHANNEL_ICON_CHANGE = 5,
    MESSAGE_CHANNEL_PINNED_MESSAGE = 6,
    MESSAGE_GUILD_MEMBER_JOIN = 7,
    MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION = 8,
    MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1 = 9,
    MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2 = 10,
    MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3 = 11,
    MESSAGE_CHANNEL_FOLLOW_ADD = 12,
    MESSAGE_GUILD_DISCOVERY_DISQUALIFIED = 14,
    MESSAGE_GUILD_DISCOVERY_REQUALIFIED = 15,
    MESSAGE_GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING = 16,
    MESSAGE_GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING = 17,
    MESSAGE_THREAD_CREATED = 18,
    MESSAGE_REPLY = 19,
    MESSAGE_CHAT_INPUT_COMMAND = 20,
    MESSAGE_THREAD_STARTER_MESSAGE = 21,
    MESSAGE_GUILD_INVITE_REMINDER = 22,
    MESSAGE_CONTEXT_MENU_COMMAND = 23
} discord_message_type;

typedef enum discord_message_flags {
    MESSAGE_FLAG_CROSSPOSTED = 1,
    MESSAGE_FLAG_IS_CROSSPOST = 2,
    MESSAGE_FLAG_SUPPRESS_EMBEDS = 4,
    MESSAGE_FLAG_SOURCE_MESSAGE_DELETED = 8,
    MESSAGE_FLAG_URGENT = 16,
    MESSAGE_FLAG_HAS_THREAD = 32,
    MESSAGE_FLAG_EPHEMERAL = 64,
    MESSAGE_FLAG_LOADING = 128,
    MESSAGE_FLAG_FAILED_TO_MENTION_SOME_ROLES_IN_THREAD = 256
} discord_message_flags;

typedef enum discord_message_activity_type {
    MESSAGE_ACTIVITY_JOIN = 1,
    MESSAGE_ACTIVITY_SPECTATE = 2,
    MESSAGE_ACTIVITY_LISTEN = 3,
    MESSAGE_ACTIVITY_JOIN_REQUEST = 5
} discord_message_activity_type;

typedef struct discord_message_activity {
    discord_message_activity_type type;
    const char *party_id;
} discord_message_activity;

typedef struct discord_message_channel_mention {
    snowflake id;
    snowflake guild_id;
    int type;
    const char *name;
} discord_message_channel_mention;

typedef struct discord_message {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    snowflake channel_id;
    snowflake guild_id;
    const discord_user *author;
    discord_member *member;
    const char *content;
    const char *timestamp;
    const char *edited_timestamp;
    bool tts;
    bool mention_everyone;
    list *mentions;
    list *mention_roles;
    list *mention_channels;
    list *attachments;
    list *embeds;
    list *reactions;
    const char *nonce;
    bool pinned;
    snowflake webhook_id;
    discord_message_type type;
    discord_message_activity *activity;
    discord_application *application;
    snowflake application_id;
    discord_message_reference *reference;
    int flags;
    const discord_message *referenced_message;
    //const discord_interaction *interaction;
    const discord_channel *thread;
    list *components;
    list *sticker_items;
} discord_message;

discord_message *message_init(discord_state *, json_object *);
bool message_update(discord_message *, json_object *);

/* API calls */
//bool message_edit(discord_message *, params);
bool message_delete(const discord_message *, const char *);

void message_free(void *);

json_object *message_reply_to_json(const discord_message_reply *);

#endif
