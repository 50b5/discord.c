#include "state.h"

static const logctx *logger = NULL;

static const char *statuses[] = {
    "offline",
    "invisible",
    "idle",
    "dnd",
    "online",

    NULL
};

discord_state *state_init(const char *token, const discord_state_options *opts){
    if (!token){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - token is NULL -- refusing to initialize\n",
            __FILE__
        );

        return NULL;
    }

    discord_state *state = calloc(1, sizeof(*state));

    if (!state){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - state object alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    if (opts){
        logger = opts->log;

        state->log = opts->log;
        state->intent = opts->intent;

        state->max_messages = opts->max_messages;
    }

    state->user_pointer = NULL;

    state->token = string_duplicate(token);

    if (!state->token){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - string_duplicate call failed for token\n",
            __FILE__
        );

        state_free(state);

        return NULL;
    }

    state->presence = json_object_new_object();

    if (!state->presence){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - presence object initialization failed\n",
            __FILE__
        );

        state_free(state);

        return NULL;
    }

    state->http = discord_http_init(state->token, NULL);

    if (!state->http){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - discord_http initialization failed\n",
            __FILE__
        );

        state_free(state);

        return NULL;
    }

    state->messages = list_init();

    if (!state->messages){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - messages list initialization failed\n",
            __FILE__
        );

        state_free(state);

        return NULL;
    }

    state->emojis = map_init();

    if (!state->emojis){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - emojis map initialization failed\n",
            __FILE__
        );

        state_free(state);

        return NULL;
    }

    state->users = map_init();

    if (!state->users){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_init() - users map initialization failed\n",
            __FILE__
        );

        state_free(state);

        return NULL;
    }

    return state;
}

json_object *state_get_presence(discord_state *state){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_get_presence() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    return state->presence;
}

const char *state_get_presence_string(discord_state *state){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_get_presence_string() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    return state->presence ? json_object_to_json_string(state->presence) : "null";
}

bool state_set_presence(discord_state *state, const discord_presence *presence){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_presence() - state is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!presence){
        json_object *obj = json_object_new_object();

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] state_set_presence() - presence object initialization failed\n",
                __FILE__
            );

            return false;
        }

        json_object_put(state->presence);

        state->presence = obj;

        return true;
    }
    else if (presence->raw_object){
        if (!json_merge_objects(presence->raw_object, state->presence)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] state_set_presence() - json_merge_objects call failed\n",
                __FILE__
            );

            return false;
        }

        return true;
    }

    bool success = state_set_presence_since(state, presence->since);

    if (!success){
        return false;
    }

    success = state_set_presence_activities(state, presence->activities);

    if (!success){
        return false;
    }

    success = state_set_presence_status(state, presence->status);

    if (!success){
        return false;
    }

    success = state_set_presence_afk(state, presence->afk);

    return success;
}

bool state_set_presence_since(discord_state *state, time_t since){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_presence_since() - state is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = json_object_new_int64(since);

    if (!obj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_since() - since object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(state->presence, "since", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_since() - json_object_object_add call failed for since\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    return true;
}

bool state_set_presence_activities(discord_state *state, const list *activities){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_presence_activities() - state is NULL\n",
            __FILE__
        );

        return false;
    }

    size_t length = list_get_length(activities);
    json_object *obj = json_object_new_array_ext(length);

    if (!obj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_activities() - activities object initialization failed\n",
            __FILE__
        );

        return false;
    }

    for (size_t index = 0; index < length; ++index){
        const discord_activity *activity = list_get_generic(activities, index);
        json_object *activityobj = json_object_get(activity_to_json(activity));

        if (json_object_array_add(obj, activityobj)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] state_set_presence_activities() - json_object_array_add call failed\n",
                __FILE__
            );

            json_object_put(activityobj);
            json_object_put(obj);

            return false;
        }
    }

    if (json_object_object_add(state->presence, "activities", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_activities() - json_object_object_add call failed\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    return true;
}

bool state_set_presence_status(discord_state *state, const char *status){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_presence_status() - state is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = NULL;

    if (status){
        bool found = false;

        for (size_t index = 0; statuses[index]; ++index){
            if (!strcmp(statuses[index], status)){
                found = true;

                break;
            }
        }

        if (!found){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] state_set_presence_status() - valid statuses are invisible, idle, dnd, online\n",
                __FILE__,
                status
            );

            return false;
        }

        obj = json_object_new_string(status);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] state_set_presence_status() - status object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(state->presence, "status", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_status() - json_object_object_add call failed\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    return true;
}

bool state_set_presence_afk(discord_state *state, bool afk){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_presence_afk() - state is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = json_object_new_boolean(afk);

    if (!obj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_afk() - afk object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(state->presence, "afk", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_presence_afk() - json_object_object_add call failed\n",
            __FILE__
        );

        return false;
    }

    return true;
}

const discord_message *state_set_message(discord_state *state, json_object *data, bool update){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_message() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }
    else if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_message() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    const char *idstr = json_object_get_string(
        json_object_object_get(data, "id")
    );

    if (!idstr){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_message() - failed to get id from data: %s\n",
            __FILE__,
            json_object_to_json_string(data)
        );

        return 0;
    }

    snowflake id = 0;
    bool success = snowflake_from_string(idstr, &id);

    if (!success){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_message() - snowflake_from_string call failed for id: %s\n",
            __FILE__,
            idstr
        );

        return 0;
    }

    discord_message *cached = NULL;

    size_t messageslen = list_get_length(state->messages);

    for (size_t index = 0; index < messageslen; ++index){
        discord_message *tmp = list_get_generic(state->messages, index);

        if (tmp->id == id){
            cached = tmp;

            break;
        }
    }

    discord_message *message = NULL;

    if (cached){
        if (update){
            if (!message_update(cached, data)){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] state_set_message() - message_update call failed\n",
                    __FILE__
                );

                message_free(message);

                return NULL;
            }
        }

        message = cached;
    }
    else {
        message = message_init(state, data);

        if (!message){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] state_set_message() - message initialization failed\n",
                __FILE__
            );

            return NULL;
        }

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*message);
        item.data = message;
        item.generic_free = message_free;

        if (!list_append(state->messages, &item)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] state_set_message() - list_append call failed\n",
                __FILE__
            );

            message_free(message);

            return NULL;
        }

        if (state->max_messages && list_get_length(state->messages) == state->max_messages){
            list_remove(state->messages, 0);
        }
    }

    return message;
}

const discord_message *state_get_message(discord_state *state, snowflake id){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_get_message() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    const discord_message *message = NULL;

    size_t messageslen = list_get_length(state->messages);

    for (size_t index = 0; index < messageslen; ++index){
        const discord_message *tmp = list_get_generic(state->messages, index);

        if (tmp->id == id){
            message = tmp;

            break;
        }
    }

    if (!message){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] state_get_message() - message %" PRIu64 " not found in cache\n",
            __FILE__,
            id
        );
    }

    return message;
}

const discord_emoji *state_set_emoji(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_emoji() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }
    else if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_emoji() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    const char *idstr = json_object_get_string(
        json_object_object_get(data, "id")
    );

    if (!idstr){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_emoji() - failed to get id from data: %s\n",
            __FILE__,
            json_object_to_json_string(data)
        );

        return NULL;
    }

    snowflake id = 0;
    bool success = snowflake_from_string(idstr, &id);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_emoji() - snowflake_from_string call failed for id: %s\n",
            __FILE__,
            idstr
        );

        return NULL;
    }

    const discord_emoji *cached = state_get_emoji(state, id);

    if (cached){
        return cached;
    }

    discord_emoji *emoji = emoji_init(state, data);

    if (!emoji){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_emoji() - emoji initialization failed\n",
            __FILE__
        );

        return false;
    }

    map_item k = {0};
    k.type = M_TYPE_UINT;
    k.size = sizeof(emoji->id);
    k.data_copy = &emoji->id;

    map_item v = {0};
    v.type = M_TYPE_GENERIC;
    v.size = sizeof(emoji);
    v.data = emoji;
    v.generic_free = emoji_free;

    if (!map_set(state->emojis, &k, &v)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_emoji() - map_set call for emojis failed\n",
            __FILE__
        );

        emoji_free(emoji);

        return NULL;
    }

    return emoji;
}

const discord_emoji *state_get_emoji(discord_state *state, snowflake id){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_get_emoji() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    size_t idsize = sizeof(id);

    if (!map_contains(state->emojis, idsize, &id)){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] state_get_emoji() - emoji %" PRIu64 " not found\n",
            __FILE__,
            id
        );

        return NULL;
    }

    return map_get_generic(state->emojis, idsize, &id);
}

const discord_user *state_set_user(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_user() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }
    else if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_user() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    const char *idstr = json_object_get_string(
        json_object_object_get(data, "id")
    );

    if (!idstr){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_set_user() - failed to get id from data: %s\n",
            __FILE__,
            json_object_to_json_string(data)
        );

        return NULL;
    }

    snowflake id = 0;
    bool success = snowflake_from_string(idstr, &id);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_user() - snowflake_from_string call failed for id: %s\n",
            __FILE__,
            idstr
        );

        return NULL;
    }

    const discord_user *cached = state_get_user(state, id);

    if (cached){
        return cached;
    }

    discord_user *user = user_init(state, data);

    if (!user){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_user() - user initialization failed\n",
            __FILE__
        );

        return false;
    }

    map_item k = {0};
    k.type = M_TYPE_UINT;
    k.size = sizeof(user->id);
    k.data_copy = &user->id;

    map_item v = {0};
    v.type = M_TYPE_GENERIC;
    v.size = sizeof(user);
    v.data = user;
    v.generic_free = user_free;

    if (!map_set(state->users, &k, &v)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] state_set_user() - map_set call for users failed\n",
            __FILE__
        );

        user_free(user);

        return NULL;
    }

    return user;
}

const discord_user *state_get_user(discord_state *state, snowflake id){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] state_get_user() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    size_t idsize = sizeof(id);

    if (!map_contains(state->users, idsize, &id)){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] state_get_user() - user %" PRIu64 " not found\n",
            __FILE__,
            id
        );

        return NULL;
    }

    return map_get_generic(state->users, idsize, &id);
}

void state_free(discord_state *state){
    if (!state){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] state_free() - state is NULL\n",
            __FILE__
        );

        return;
    }

    discord_http_free(state->http);

    json_object_put(state->presence);

    list_free(state->messages);
    map_free(state->emojis);
    map_free(state->users);

    free(state->token);
    free(state);
}
