#ifndef STATE_H
#define STATE_H

#include "json_utils.h"
#include "list.h"
#include "log.h"
#include "map.h"
#include "str.h"

#include "snowflake.h"

typedef struct discord_activity discord_activity;
typedef struct discord_application discord_application;
typedef struct discord_channel discord_channel;
typedef struct discord_embed discord_embed;
typedef struct discord_emoji discord_emoji;
typedef struct discord_http discord_http;
typedef struct discord_member discord_member;
typedef struct discord_message discord_message;
typedef struct discord_message_reply discord_message_reply;
typedef struct discord_state discord_state;
typedef struct discord_team discord_team;
typedef struct discord_user discord_user;

#include "activity.h"
#include "application.h"
#include "attachment.h"
#include "channel.h"
#include "embed.h"
#include "emoji.h"
#include "http.h"
#include "member.h"
#include "message.h"
#include "reaction.h"
#include "team.h"
#include "user.h"

#define DISCORD_LIBRARY_NAME "discord.c"
#define DISCORD_LIBRARY_URL "https://github.com/010000010100111001010101/discord.c"
#define DISCORD_LIBRARY_VERSION "0"
#define DISCORD_LIBRARY_OS "Linux"

#define DISCORD_API "https://discord.com/api/v10"
#define DISCORD_CDN "https://cdn.discordapp.com"
#define DISCORD_USER_AGENT ("DiscordBot (" DISCORD_LIBRARY_URL " " DISCORD_LIBRARY_VERSION ")")

#define DISCORD_GATEWAY_VERSION 9
#define DISCORD_GATEWAY_PORT 443
#define DISCORD_GATEWAY_ENCODING "json"
#define DISCORD_GATEWAY_IDENTIFY_LIMIT 1000
#define DISCORD_GATEWAY_HEARTBEAT_JITTER 0.5
#define DISCORD_GATEWAY_RATE_LIMIT_INTERVAL 60
#define DISCORD_GATEWAY_RATE_LIMIT_COUNT 110
#define DISCORD_GATEWAY_LWS_LOG_LEVEL (LLL_ERR | LLL_WARN | LLL_NOTICE)

typedef enum discord_gateway_intents {
    INTENT_GUILDS = 1,
    INTENT_GUILD_MEMBERS = 2,
    INTENT_GUILD_BANS = 4,
    INTENT_GUILD_EMOJIS_AND_STICKERS = 8,
    INTENT_GUILD_INTEGRATIONS = 16,
    INTENT_GUILD_WEBHOOKS = 32,
    INTENT_GUILD_INVITES = 64,
    INTENT_GUILD_VOICE_STATES = 128,
    INTENT_GUILD_PRESENCES = 256,
    INTENT_GUILD_MESSAGES = 512,
    INTENT_GUILD_MESSAGE_REACTIONS = 1024,
    INTENT_GUILD_MESSAGE_TYPING = 2048,
    INTENT_DIRECT_MESSAGES = 4096,
    INTENT_DIRECT_MESSAGE_REACTIONS = 8192,
    INTENT_DIRECT_MESSAGE_TYPING = 16384,
    INTENT_MESSAGE_CONTENT = 32768,
    INTENT_GUILD_SCHEDULED_EVENTS = 65536,

    INTENT_ALL = 131071
} discord_gateway_intents;

typedef struct discord_presence {
    json_object *raw_object;

    time_t since;
    const list *activities;
    const char *status;
    bool afk;
} discord_presence;

typedef struct discord_state_options {
    const logctx *log;
    discord_gateway_intents intent;

    size_t max_messages;
} discord_state_options;

typedef struct discord_state {
    char *token;
    const logctx *log;
    discord_http *http;

    discord_gateway_intents intent;
    void *event_context;
    const void **user_pointer;

    const discord_user *user;
    json_object *presence;

    list *messages;
    size_t max_messages;

    map *emojis;
    map *users;
} discord_state;

discord_state *state_init(const char *, const discord_state_options *);

json_object *state_get_presence(discord_state *);
const char *state_get_presence_string(discord_state *);

bool state_set_presence(discord_state *, const discord_presence *);
bool state_set_presence_since(discord_state *, time_t);
bool state_set_presence_activities(discord_state *, const list *);
bool state_set_presence_status(discord_state *, const char *);
bool state_set_presence_afk(discord_state *, bool);

const discord_message *state_set_message(discord_state *, json_object *, bool);
const discord_message *state_get_message(discord_state *, snowflake);

const discord_emoji *state_set_emoji(discord_state *, json_object *);
const discord_emoji *state_get_emoji(discord_state *, snowflake);

const discord_user *state_set_user(discord_state *, json_object *);
const discord_user *state_get_user(discord_state *, snowflake);

void state_free(discord_state *);

#endif
