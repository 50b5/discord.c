#include "guild.h"

static const logctx *logger = NULL;

static bool construct_guild(discord_guild *guild){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(guild->raw_object);
    struct json_object_iterator end = json_object_iter_end(guild->raw_object);

    while (!json_object_iter_equal(&curr, &end)){
        const char *key = json_object_iter_peek_name(&curr);
        json_object *valueobj = json_object_iter_peek_value(&curr);
        json_type type = json_object_get_type(valueobj);

        bool skip = false;

        if (!valueobj || type == json_type_null){
            skip = true;
        }
        else if (type == json_type_array){
            skip = !json_object_array_length(valueobj);
        }

        if (skip){
            json_object_iter_next(&curr);

            continue;
        }

        if (!strcmp(key, "id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->id);
        }
        else if (!strcmp(key, "name")){
            guild->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "icon")){
            guild->icon = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "icon_hash")){
            guild->icon_hash = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "splash")){
            guild->splash = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "discovery_splash")){
            guild->discovery_splash = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "owner")){
            guild->owner = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "owner_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->owner_id);
        }
        else if (!strcmp(key, "permissions")){
            guild->permissions = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "afk_channel_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->afk_channel_id);
        }
        else if (!strcmp(key, "afk_timeout")){
            guild->afk_timeout = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "widget_enabled")){
            guild->widget_enabled = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "widget_channel_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->widget_channel_id);
        }
        else if (!strcmp(key, "verification_level")){
            guild->verification_level = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "default_message_notifications")){
            guild->default_message_notifications = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "explicit_content_filter")){
            guild->explicit_content_filter = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "roles")){
            //success = construct_guild_roles(guild, valueobj);
        }
        else if (!strcmp(key, "emojis")){
            //success = construct_guild_emojis(guild, valueobj);
        }
        else if (!strcmp(key, "features")){
            guild->features = json_array_to_list(valueobj);

            success = guild->features;
        }
        else if (!strcmp(key, "mfa_level")){
            guild->mfa_level = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "application_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->application_id);
        }
        else if (!strcmp(key, "system_channel_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->system_channel_id);
        }
        else if (!strcmp(key, "system_channel_flags")){
            guild->system_channel_flags = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "rules_channel_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->rules_channel_id);
        }
        else if (!strcmp(key, "joined_at")){
            guild->joined_at = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "large")){
            guild->large = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "unavailable")){
            guild->unavailable = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "member_count")){
            guild->member_count = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "voice_states")){
            //success = construct_guild_voice_states(guild, valueobj);
        }
        else if (!strcmp(key, "members")){
            //success = construct_guild_members(guild, valueobj);
        }
        else if (!strcmp(key, "channels")){
            //success = construct_guild_channels(guild, valueobj);
        }
        else if (!strcmp(key, "threads")){
            //success = construct_guild_threads(guild, valueobj);
        }
        else if (!strcmp(key, "max_presences")){
            guild->max_presences = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "max_members")){
            guild->max_members = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "vanity_url_code")){
            guild->vanity_url_code = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "description")){
            guild->description = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "banner")){
            guild->banner = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "premium_tier")){
            guild->premium_tier = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "premium_subscription_count")){
            guild->premium_subscription_count = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "preferred_locale")){
            guild->preferred_locale = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "public_updates_channel_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &guild->public_updates_channel_id);
        }
        else if (!strcmp(key, "max_video_channel_users")){
            guild->max_video_channel_users = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "approximate_member_count")){
            guild->approximate_member_count = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "approximate_presence_count")){
            guild->approximate_presence_count = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "welcome_screen")){
            //success = construct_guild_welcome_screen(guild, valueobj);
        }
        else if (!strcmp(key, "nsfw_level")){
            guild->nsfw_level = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "stage_instances")){
            //success = construct_guild_stage_instances(guild, valueobj);
        }
        else if (!strcmp(key, "stickers")){
            //success = construct_guild_stickers(guild, valueobj);
        }
        else if (!strcmp(key, "guild_scheduled_events")){
            //success = construct_guild_scheduled_events(guild, valueobj);
        }
        else if (!strcmp(key, "premium_progress_bar_enabled")){
            guild->premium_progress_bar_enabled = json_object_get_boolean(valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_guild() - failed to set %s with value: %s\n",
                __FILE__,
                key,
                json_object_to_json_string(valueobj)
            );

            break;
        }

        json_object_iter_next(&curr);
    }

    return success;
}

discord_guild *guild_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] guild_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] guild_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_guild *guild = calloc(1, sizeof(*guild));

    if (!guild){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] guild_init() - guild alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    guild->state = state;
    guild->raw_object = json_object_get(data);

    if (!construct_guild(guild)){
        guild_free(guild);

        return NULL;
    }

    return guild;
}

void guild_free(void *ptr){
    discord_guild *guild = ptr;

    if (!guild){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] guild_free() - guild is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(guild->raw_object);

    map_free(guild->roles);
    list_free(guild->emojis);
    list_free(guild->features);
    list_free(guild->voice_states);

    map_free(guild->members);
    map_free(guild->channels);
    map_free(guild->threads);

    list_free(guild->stage_instances);
    list_free(guild->stickers);
    list_free(guild->guild_scheduled_events);

    free(guild);
}
