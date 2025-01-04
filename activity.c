#include "activity.h"

static const logctx *logger = NULL;

static bool construct_activity_buttons_from_json(discord_activity *activity, json_object *data){
    if (activity->buttons){
        list_empty(activity->buttons);
    }
    else {
        activity->buttons = list_init();

        if (!activity->buttons){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_activity_buttons_from_json() - buttons initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *valueobj = json_object_array_get_idx(data, index);

        discord_activity_button *button = calloc(1, sizeof(*button));

        if (!button){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_activity_buttons_from_json() - button alloc failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        json_object *obj = json_object_object_get(valueobj, "label");
        button->label = json_object_get_string(obj);

        obj = json_object_object_get(valueobj, "url");
        button->url = json_object_get_string(obj);

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(button);
        item.data = button;

        success = list_append(activity->buttons, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_activity_buttons_from_json() - list_append call failed\n",
                __FILE__
            );

            free(button);

            break;
        }
    }

    return success;
}

static bool construct_activity_from_json(discord_activity *activity){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(activity->raw_object);
    struct json_object_iterator end = json_object_iter_end(activity->raw_object);

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

        if (!strcmp(key, "name")){
            activity->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "type")){
            activity->type = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "url")){
            activity->url = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "created_at")){
            activity->created_at = json_object_get_int64(valueobj);
        }
        else if (!strcmp(key, "timestamps")){
            if (!activity->timestamps){
                activity->timestamps = calloc(1, sizeof(*activity->timestamps));

                if (!activity->timestamps){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_activity_from_json() - timestamps alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "start");
            activity->timestamps->start = json_object_get_int64(obj);

            obj = json_object_object_get(valueobj, "end");
            activity->timestamps->end = json_object_get_int64(obj);
        }
        else if (!strcmp(key, "application_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &activity->application_id);
        }
        else if (!strcmp(key, "details")){
            activity->details = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "state")){
            activity->status = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "emoji")){
            activity->emoji = state_set_emoji(activity->state, valueobj);

            success = activity->emoji;
        }
        else if (!strcmp(key, "party")){
            if (!activity->party){
                activity->party = calloc(1, sizeof(*activity->party));

                if (!activity->party){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_activity_from_json() - party alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "id");
            activity->party->id = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "size");
            activity->party->size = json_array_to_list(obj);

            success = activity->party->size;
        }
        else if (!strcmp(key, "assets")){
            if (!activity->assets){
                activity->assets = calloc(1, sizeof(*activity->assets));

                if (!activity->assets){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_activity_from_json() - assets alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "large_image");
            activity->assets->large_image = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "large_text");
            activity->assets->large_text = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "small_image");
            activity->assets->small_image = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "small_text");
            activity->assets->small_text = json_object_get_string(obj);
        }
        else if (!strcmp(key, "secrets")){
            if (!activity->secrets){
                activity->secrets = calloc(1, sizeof(*activity->secrets));

                if (!activity->secrets){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_activity_from_json() - secrets alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "join");
            activity->secrets->join = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "spectate");
            activity->secrets->spectate = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "match");
            activity->secrets->match = json_object_get_string(obj);
        }
        else if (!strcmp(key, "instance")){
            activity->instance = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "flags")){
            activity->flags = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "buttons")){
            success = construct_activity_buttons_from_json(activity, valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_activity_from_json() - failed to set %s with value: %s\n",
                __FILE__,
                key,
                json_object_to_json_string(valueobj)
            );

            break;
        }

        json_object_iter_next(&curr);
    }

    return true;
}

discord_activity_type activity_type_from_string(const char *input){
    discord_activity_type type = 0;

    if (!strcmp(input, "playing") || !strcmp(input, "game")){
        type = ACTIVITY_GAME;
    }
    else if (!strcmp(input, "streaming")){
        type = ACTIVITY_STREAMING;
    }
    else if (!strcmp(input, "listening")){
        type = ACTIVITY_LISTENING;
    }
    else if (!strcmp(input, "watching")){
        type = ACTIVITY_WATCHING;
    }
    else if (!strcmp(input, "custom")){
        type = ACTIVITY_CUSTOM;
    }
    else if (!strcmp(input, "competing")){
        type = ACTIVITY_COMPETING;
    }

    return type;
}

discord_activity *activity_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] activity_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    discord_activity *activity = calloc(1, sizeof(*activity));

    if (!activity){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] activity_init() - activity alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    activity->state = state;

    if (data){
        activity->raw_object = json_object_get(data);

        if (!construct_activity_from_json(activity)){
            activity_free(activity);

            return NULL;
        }
    }
    else {
        activity->raw_object = json_object_new_object();

        if (!activity->raw_object){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] activity_init() - raw_object initialization failed\n",
                __FILE__
            );

            activity_free(activity);

            return NULL;
        }
    }

    return activity;
}

json_object *activity_to_json(const discord_activity *activity){
    if (!activity){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] activity_to_json() - activity is NULL\n",
            __FILE__
        );

        return NULL;
    }

    return activity->raw_object;
}

bool activity_set_name(discord_activity *activity, const char *name){
    if (!activity){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] activity_set_name() - activity is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = NULL;

    if (name){
        obj = json_object_new_string(name);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] activity_set_name() - name object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(activity->raw_object, "name", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] activity_set_name() - json_object_object_add call failed\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    activity->name = json_object_get_string(obj);

    return true;
}

bool activity_set_type(discord_activity *activity, discord_activity_type type){
    if (!activity){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] activity_set_type() - activity is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = json_object_new_int(type);

    if (!obj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] activity_set_type() - type object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(activity->raw_object, "type", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] activity_set_type() - json_object_object_add call failed\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    activity->type = json_object_get_int(obj);

    return true;
}

bool activity_set_url(discord_activity *activity, const char *url){
    if (!activity){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] activity_set_url() - activity is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = NULL;

    if (url){
        obj = json_object_new_string(url);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] activity_set_url() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(activity->raw_object, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] activity_set_url() - json_object_object_add call failed\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    activity->url = json_object_get_string(obj);

    return true;
}

void activity_free(void *ptr){
    discord_activity *activity = ptr;

    if (!activity){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] activity_free() - activity is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(activity->raw_object);

    free(activity->party);
    free(activity->assets);
    free(activity->secrets);
    list_free(activity->buttons);

    free(activity);
}
