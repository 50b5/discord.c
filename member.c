#include "member.h"

static const logctx *logger = NULL;

static bool construct_member_roles(discord_member *member, json_object *data){
    if (member->roles){
        list_empty(member->roles);
    }
    else {
        member->roles = list_init();

        if (!member->roles){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_member_roles() - roles initialization failed\n",
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
                "[%s] construct_member_roles() - snowflake_from_string call failed for %s\n",
                __FILE__,
                json_object_to_json_string(obj)
            );

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_UINT;
        item.size = sizeof(id);
        item.data_copy = &id;

        success = list_append(member->roles, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_member_roles() - list_append call failed\n",
                __FILE__
            );

            break;
        }
    }

    return success;
}

static bool construct_member(discord_member *member){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(member->raw_object);
    struct json_object_iterator end = json_object_iter_end(member->raw_object);

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

        if (!strcmp(key, "user")){
            member->user = state_set_user(
                member->state,
                valueobj
            );

            success = member->user;
        }
        else if (!strcmp(key, "nick")){
            member->nick = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "avatar")){
            member->avatar = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "roles")){
            success = construct_member_roles(member, valueobj);
        }
        else if (!strcmp(key, "joined_at")){
            member->joined_at = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "premium_since")){
            member->premium_since = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "deaf")){
            member->deaf = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "mute")){
            member->mute = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "pending")){
            member->pending = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "permissions")){
            member->permissions = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "communication_disabled_until")){
            member->communication_disabled_until = json_object_get_string(valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_member() - failed to set %s with value: %s\n",
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

discord_member *member_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] member_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] member_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_member *member = calloc(1, sizeof(*member));

    if (!member){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] member_init() - member object alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    member->state = state;
    member->raw_object = json_object_get(data);

    if (!construct_member(member)){
        member_free(member);

        return NULL;
    }

    return member;
}

void member_free(void *memberptr){
    discord_member *member = memberptr;

    if (!member){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] member_free() - member is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(member->raw_object);

    list_free(member->roles);

    free(member);
}
