#ifndef APPLICATION_H
#define APPLICATION_H

#include "snowflake.h"
#include "state.h"
#include "team.h"
#include "user.h"

typedef struct discord_install_params {
    list *scopes;
    const char *permissions;
} discord_install_params;

typedef enum discord_application_flags {
    APPLICATION_GATEWAY_PRESENCE = 4096,
    APPLICATION_GATEWAY_PRESENCE_LIMITED = 8192,
    APPLICATION_GATEWAY_GUILD_MEMBERS = 16384,
    APPLICATION_GATEWAY_GUILD_MEMBERS_LIMITED = 32768,
    APPLICATION_VERIFICATION_PENDING_GUILD_LIMIT = 65536,
    APPLICATION_EMBEDDED = 131072,
    APPLICATION_GATEWAY_MESSAGE_CONTENT = 262144,
    APPLICATION_GATEWAY_MESSAGE_CONTENT_LIMITED = 524288
} discord_application_flags;

typedef struct discord_application {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    const char *name;
    const char *icon;
    const char *description;
    list *rpc_origins;
    bool bot_public;
    bool bot_require_code_grant;
    const char *terms_of_service_url;
    const char *privacy_policy_url;
    const discord_user *owner;
    const char *verify_key;
    discord_team *team;
    snowflake guild_id;
    snowflake primary_sku_id;
    const char *slug;
    const char *cover_image;
    discord_application_flags flags;
    list *tags;
    discord_install_params *install_params;
    const char *custom_install_url;
} discord_application;

discord_application *application_init(discord_state *, json_object *);

void application_free(void *);

#endif
