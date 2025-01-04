#include "team.h"

static const logctx *logger = NULL;

static bool construct_team_member(discord_state *state, discord_team_member *member, json_object *data){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(data);
    struct json_object_iterator end = json_object_iter_end(data);

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

        if (!strcmp(key, "membership_state")){
            member->membership_state = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "permissions")){
            member->permissions = json_array_to_list(valueobj);

            success = member->permissions;
        }
        else if (!strcmp(key, "team_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &member->team_id);
        }
        else if (!strcmp(key, "user")){
            member->user = state_set_user(state, valueobj);

            success = member->user;
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_team_member() - failed to set %s with value: %s\n",
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

static bool construct_team_members(discord_team *team, json_object *data){
    if (team->members){
        list_empty(team->members);
    }
    else {
        team->members = list_init();

        if (!team->members){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_team_members() - members initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *obj = json_object_array_get_idx(data, index);

        discord_team_member *member = malloc(sizeof(*member));

        if (!member){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_team_members() - member alloc failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        success = construct_team_member(team->state, member, obj);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_team_members() - construct_team_member call failed\n",
                __FILE__
            );

            team_member_free(member);

            success = false;

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*member);
        item.data = member;
        item.generic_free = team_member_free;

        success = list_append(team->members, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_team_members() - list_append call failed\n",
                __FILE__
            );

            break;
        }
    }

    return success;
}

static bool construct_team(discord_team *team){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(team->raw_object);
    struct json_object_iterator end = json_object_iter_end(team->raw_object);

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

        if (!strcmp(key, "icon")){
            team->icon = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &team->id);
        }
        else if (!strcmp(key, "members")){
            success = construct_team_members(team, valueobj);
        }
        else if (!strcmp(key, "name")){
            team->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "owner_user_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &team->owner_user_id);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_team() - failed to set %s with value: %s\n",
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

discord_team *team_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] team_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] team_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_team *team = calloc(1, sizeof(*team));

    if (!team){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] team_init() - team alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    team->state = state;
    team->raw_object = json_object_get(data);

    if (!construct_team(team)){
        team_free(team);

        return NULL;
    }

    return team;
}

void team_member_free(void *memberptr){
    discord_team_member *member = memberptr;

    if (!member){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] team_member_free() - team member is NULL\n",
            __FILE__
        );

        return;
    }

    list_free(member->permissions);

    free(member);
}

void team_free(void *teamptr){
    discord_team *team = teamptr;

    if (!team){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] team_free() - team is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(team->raw_object);

    list_free(team->members);

    free(team);
}
