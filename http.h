#ifndef DISCORD_HTTP_H
#define DISCORD_HTTP_H

#include "map.h"

#include "snowflake.h"
#include "state.h"

#include <json-c/json.h>

typedef enum discord_http_method {
    DISCORD_HTTP_GET,
    DISCORD_HTTP_DELETE,
    DISCORD_HTTP_PATCH,
    DISCORD_HTTP_POST,
    DISCORD_HTTP_PUT
} discord_http_method;

typedef struct discord_http_request_options {
    json_object *data;
    const char *reason;
} discord_http_request_options;

typedef struct discord_http_response {
    long status;
    map *headers;
    json_object *data;
} discord_http_response;

typedef struct discord_http_options {
    const logctx *log;
} discord_http_options;

typedef struct discord_http {
    const char *token;

    bool ratelimited;
    bool globalratelimit;
    map *buckets;
} discord_http;

discord_http *discord_http_init(const char *, const discord_http_options *);

discord_http_response *discord_http_request(discord_http *, discord_http_method, const char *, const discord_http_request_options *);

/*
 * API calls by type
 */

/* General */
discord_http_response *discord_http_get_gateway(discord_http *);
discord_http_response *discord_http_get_bot_gateway(discord_http *);

discord_http_response *discord_http_get_current_application_information(discord_http *);

/* User */
discord_http_response *discord_http_get_current_user(discord_http *);
discord_http_response *discord_http_get_user(discord_http *, snowflake);
discord_http_response *discord_http_modify_current_user(discord_http *, const char *, const char *);
discord_http_response *discord_http_get_current_user_guilds(discord_http *, snowflake, snowflake, int);
discord_http_response *discord_http_get_current_user_guild_member(discord_http *, snowflake);
discord_http_response *discord_http_leave_guild(discord_http *, snowflake);
discord_http_response *discord_http_create_dm(discord_http *, snowflake);
//discord_http_response *discord_http_create_group_dm(discord_http *, const json_object *);
discord_http_response *discord_http_get_user_connections(discord_http *);

/* Channel */
discord_http_response *discord_http_trigger_typing_indicator(discord_http *, snowflake);
discord_http_response *discord_http_follow_news_channel(discord_http *, snowflake, json_object *);

discord_http_response *discord_http_get_channel(discord_http *, snowflake);
discord_http_response *discord_http_edit_channel(discord_http *, snowflake, json_object *, const char *);
discord_http_response *discord_http_delete_channel(discord_http *, snowflake, const char *);

discord_http_response *discord_http_edit_channel_permissions(discord_http *, snowflake, snowflake, json_object *, const char *);
discord_http_response *discord_http_delete_channel_permission(discord_http *, snowflake, snowflake, const char *);

discord_http_response *discord_http_get_channel_invites(discord_http *, snowflake);
discord_http_response *discord_http_create_channel_invite(discord_http *, snowflake, json_object *, const char *);

discord_http_response *discord_http_get_pinned_messages(discord_http *, snowflake);
discord_http_response *discord_http_pin_message(discord_http *, snowflake, snowflake, const char *);
discord_http_response *discord_http_unpin_message(discord_http *, snowflake, snowflake, const char *);

/* Message */
discord_http_response *discord_http_get_channel_messages(discord_http *, snowflake, json_object *);
discord_http_response *discord_http_get_channel_message(discord_http *, snowflake, snowflake);

discord_http_response *discord_http_crosspost_message(discord_http *, snowflake, snowflake);

discord_http_response *discord_http_create_message(discord_http *, snowflake, json_object *);
discord_http_response *discord_http_edit_message(discord_http *, snowflake, snowflake, json_object *);
discord_http_response *discord_http_delete_message(discord_http *, snowflake, snowflake, const char *);
discord_http_response *discord_http_bulk_delete_messages(discord_http *, snowflake, json_object *, const char *);

discord_http_response *discord_http_get_reactions(discord_http *, snowflake, snowflake, const char *);
discord_http_response *discord_http_create_reaction(discord_http *, snowflake, snowflake, const char *);
discord_http_response *discord_http_delete_own_reaction(discord_http *, snowflake, snowflake, const char *);
discord_http_response *discord_http_delete_user_reaction(discord_http *, snowflake, snowflake, snowflake, const char *);
discord_http_response *discord_http_delete_all_reactions(discord_http *, snowflake, snowflake);
discord_http_response *discord_http_delete_all_reactions_for_emoji(discord_http *, snowflake, snowflake, const char *);

void discord_http_response_free(discord_http_response *);
void discord_http_free(discord_http *);

#endif
