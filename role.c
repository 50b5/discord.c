#include "role.h"

static const logctx *logger = NULL;

static bool construct_role(discord_role *role){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(role->raw_object);
    struct json_object_iterator end = json_object_iter_end(role->raw_object);

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

            success = snowflake_from_string(objstr, &role->id);
        }
        else if (!strcmp(key, "name")){
            role->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "color")){
            role->color = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "hoist")){
            role->hoist = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "icon")){
            role->icon = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "unicode_emoji")){
            role->unicode_emoji = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "position")){
            role->position = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "permissions")){
            role->permissions = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "managed")){
            role->managed = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "mentionable")){
            role->mentionable = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "tags")){
            if (!role->tags){
                role->tags = calloc(1, sizeof(*role->tags));

                if (!role->tags){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_role() - tags alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "bot_id");
            const char *objstr = json_object_get_string(obj);
            success = snowflake_from_string(objstr, &role->tags->bot_id);

            if (!success){
                log_write(
                    logger,
                    LOG_WARNING,
                    "[%s] construct_role() - snowflake_from_string call failed for bot_id\n",
                    __FILE__
                );

                break;
            }

            obj = json_object_object_get(valueobj, "integration_id");
            objstr = json_object_get_string(obj);
            success = snowflake_from_string(objstr, &role->tags->integration_id);

            if (!success){
                log_write(
                    logger,
                    LOG_WARNING,
                    "[%s] construct_role() - snowflake_from_string call failed for integration_id\n",
                    __FILE__
                );

                break;
            }

            obj = json_object_object_get(valueobj, "premium_subscriber");
            role->tags->premium_subscriber = json_object_get_boolean(obj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_role() - failed to set %s with value: %s\n",
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

discord_role *role_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] role_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    discord_role *role = calloc(1, sizeof(*role));

    if (!role){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] role_init() - role alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    role->state = state;
    role->raw_object = json_object_get(data);

    if (!construct_role(role)){
        role_free(role);

        return NULL;
    }

    return role;
}

void role_free(void *roleptr){
    discord_role *role = roleptr;

    if (!role){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] role_free() - role is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(role->raw_object);

    free(role->tags);
    free(role);
}
