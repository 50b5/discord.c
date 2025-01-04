#include "channel.h"

static const logctx *logger = NULL;

static bool construct_channel(discord_channel *channel){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(channel->raw_object);
    struct json_object_iterator end = json_object_iter_end(channel->raw_object);

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

            success = snowflake_from_string(objstr, &channel->id);
        }
        else if (!strcmp(key, "type")){
            channel->type = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "guild_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &channel->guild_id);
        }
        else if (!strcmp(key, "position")){
            channel->position = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "permission_overwrites")){
            // --- BOOKMARK --- permissions/permission_overwrites.c?
            //success = construct_channel_permission_overwrites(channel, valueobj);
        }
        else if (!strcmp(key, "name")){
            channel->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "topic")){
            channel->topic = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "nsfw")){
            channel->nsfw = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "last_message_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &channel->last_message_id);
        }
        else if (!strcmp(key, "bitrate")){
            channel->bitrate = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "user_limit")){
            channel->user_limit = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "rate_limit_per_user")){
            channel->rate_limit_per_user = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "recipients")){
            //success = construct_channel_recipients(channel, valueobj);
        }
        else if (!strcmp(key, "icon")){
            channel->icon = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "owner_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &channel->owner_id);
        }
        else if (!strcmp(key, "application_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &channel->application_id);
        }
        else if (!strcmp(key, "parent_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &channel->parent_id);
        }
        else if (!strcmp(key, "last_pin_timestamp")){
            channel->last_pin_timestamp = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "rtc_region")){
            channel->rtc_region = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "video_quality_mode")){
            channel->video_quality_mode = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "message_count")){
            channel->message_count = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "member_count")){
            channel->member_count = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "thread_metadata")){
            // create thread metadata
        }
        else if (!strcmp(key, "member")){
            // create thread member
        }
        else if (!strcmp(key, "default_auto_archive_duration")){
            channel->default_auto_archive_duration = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "permissions")){
            channel->permissions = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "flags")){
            channel->flags = json_object_get_int(valueobj);
        }

        json_object_iter_next(&curr);
    }

    return success;
}

discord_channel *channel_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] channel_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] channel_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_channel *channel = calloc(1, sizeof(*channel));

    if (!channel){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] channel_init() - channel object alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    channel->state = state;
    channel->raw_object = json_object_get(data);

    if (!construct_channel(channel)){
        channel_free(channel);

        return NULL;
    }

    return channel;
}

bool channel_send_message(discord_channel *channel, const discord_message_reply *message){
    if (!channel){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] channel_send_message() - channel is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!message){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] channel_send_message() - message is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!message->content && !message->embed && !message->embeds && !message->sticker_ids){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] channel_send_message() - one of content, file, embeds, sticker_ids required\n",
            __FILE__
        );

        return true;
    }

    json_object *data = message_reply_to_json(message);

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] channel_send_message() - message_reply_to_json call failed\n",
            __FILE__
        );

        return false;
    }

    discord_http_response *res = discord_http_create_message(channel->state->http, channel->id, data);

    json_object_put(data);

    bool success = true;

    if (!res){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] channel_send_message() - discord_http_create_message call failed\n",
            __FILE__
        );

        success = channel->state->http->ratelimited;
    }
    else if (res->status != 200){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] channel_send_message() - request failed: %s\n",
            __FILE__,
            json_object_to_json_string(res->data)
        );
    }

    discord_http_response_free(res);

    return success;
}

void channel_free(void *ptr){
    discord_channel *channel = ptr;

    if (!channel){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] channel_free() - channel is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(channel->raw_object);

    list_free(channel->permission_overwrites);
    list_free(channel->recipients);

    free(channel);
}
