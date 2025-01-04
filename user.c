#include "user.h"

static const logctx *logger = NULL;

static bool construct_user(discord_user *user){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(user->raw_object);
    struct json_object_iterator end = json_object_iter_end(user->raw_object);

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

            success = snowflake_from_string(objstr, &user->id);
        }
        else if (!strcmp(key, "username")){
            user->username = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "discriminator")){
            user->discriminator = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "avatar")){
            user->avatar = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "bot")){
            user->bot = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "system")){
            user->system = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "mfa_enabled")){
            user->mfa_enabled = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "banner")){
            user->banner = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "accent_color")){
            user->accent_color = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "locale")){
            user->locale = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "verified")){
            user->verified = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "email")){
            user->email = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "flags")){
            user->flags = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "premium_type")){
            user->premium_type = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "public_flags")){
            user->public_flags = json_object_get_int(valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_user() - failed to set %s with value: %s\n",
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

discord_user *user_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] user_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] user_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_user *user = calloc(1, sizeof(*user));

    if (!user){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] user_init() - user object alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    user->state = state;
    user->raw_object = json_object_get(data);

    if (!construct_user(user)){
        user_free(user);

        return NULL;
    }

    return user;
}

time_t user_get_creation_time(const discord_user *user){
    if (!user){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] user_get_creation_time() - user is NULL\n",
            __FILE__
        );

        return 0;
    }

    return snowflake_get_creation_time(user->id);
}

void user_free(void *userptr){
    discord_user *user = userptr;

    if (!user){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] user_free() - user is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(user->raw_object);

    free(user);
}
