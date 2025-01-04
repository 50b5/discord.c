#include "attachment.h"

static const logctx *logger = NULL;

static bool construct_attachment(discord_attachment *attachment){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(attachment->raw_object);
    struct json_object_iterator end = json_object_iter_end(attachment->raw_object);

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

            success = snowflake_from_string(objstr, &attachment->id);
        }
        else if (!strcmp(key, "filename")){
            attachment->filename = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "description")){
            attachment->description = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "content_type")){
            attachment->content_type = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "size")){
            attachment->size = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "url")){
            attachment->url = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "proxy_url")){
            attachment->proxy_url = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "height")){
            attachment->height = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "width")){
            attachment->width = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "ephemeral")){
            attachment->ephemeral = json_object_get_boolean(valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_attachment() - failed to set %s with value: %s\n",
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

discord_attachment *attachment_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] attachment_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] attachment_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_attachment *attachment = calloc(1, sizeof(*attachment));

    if (!attachment){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] attachment_init() - attachment alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    attachment->state = state;
    attachment->raw_object = json_object_get(data);

    if (!construct_attachment(attachment)){
        attachment_free(attachment);

        return NULL;
    }

    return attachment;
}

void attachment_free(void *ptr){
    discord_attachment *attachment = ptr;

    if (!attachment){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] attachment_free() - attachment is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(attachment->raw_object);

    free(attachment);
}
