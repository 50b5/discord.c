#include "emoji.h"

static const logctx *logger = NULL;

static bool construct_emoji_roles(discord_emoji *emoji, json_object *data){
    if (emoji->roles){
        list_empty(emoji->roles);
    }
    else {
        emoji->roles = list_init();

        if (!emoji->roles){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_emoji_roles() - roles initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *obj = json_object_array_get_idx(data, index);
        const char *objstr = json_object_get_string(obj);

        snowflake id = 0;
        success = snowflake_from_string(objstr, &id);

        if (!success){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] construct_emoji_roles() - snowflake_from_string call failed for %s\n",
                __FILE__,
                json_object_to_json_string(obj)
            );

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_UINT;
        item.size = sizeof(id);
        item.data_copy = &id;

        success = list_append(emoji->roles, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_emoji_roles() - list_append call failed\n",
                __FILE__
            );

            break;
        }
    }

    return success;
}

static bool construct_emoji(discord_emoji *emoji){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(emoji->raw_object);
    struct json_object_iterator end = json_object_iter_end(emoji->raw_object);

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

            success = snowflake_from_string(objstr, &emoji->id);
        }
        else if (!strcmp(key, "name")){
            emoji->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "roles")){
            success = construct_emoji_roles(emoji, valueobj);
        }
        else if (!strcmp(key, "user")){
            emoji->user = state_set_user(emoji->state, valueobj);

            success = emoji->user;
        }
        else if (!strcmp(key, "require_colons")){
            emoji->require_colons = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "managed")){
            emoji->managed = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "animated")){
            emoji->animated = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "available")){
            emoji->available = json_object_get_boolean(valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_emoji() - failed to set %s with value: %s\n",
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

discord_emoji *emoji_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] emoji_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] emoji_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_emoji *emoji = calloc(1, sizeof(*emoji));

    if (!emoji){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] emoji_init() - emoji alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    emoji->state = state;
    emoji->raw_object = json_object_get(data);

    if (!construct_emoji(emoji)){
        emoji_free(emoji);

        return NULL;
    }

    return emoji;
}

void emoji_free(void *ptr){
    discord_emoji *emoji = ptr;

    if (!emoji){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] emoji_free() - emoji is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(emoji->raw_object);

    list_free(emoji->roles);

    free(emoji);
}
