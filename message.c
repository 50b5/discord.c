#include "message.h"

static const logctx *logger = NULL;

static bool construct_message_activity(discord_message *message, json_object *data){
    message->activity = calloc(1, sizeof(*message->activity));

    if (!message->activity){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] construct_message_activity() - activity alloc failed\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = json_object_object_get(data, "type");
    message->activity->type = json_object_get_int(obj);

    obj = json_object_object_get(data, "party_id");
    message->activity->party_id = json_object_get_string(obj);

    return true;
}

static bool construct_message_reference(discord_message *message, json_object *data){
    message->reference = calloc(1, sizeof(*message->reference));

    if (!message->reference){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] construct_message_reference() - reference alloc failed\n",
            __FILE__
        );

        return false;
    }

    bool success = true;

    json_object *obj = json_object_object_get(data, "message_id");
    const char *objstr = json_object_get_string(obj);
    success = snowflake_from_string(objstr, &message->reference->message_id);

    if (!success){
        return false;
    }

    obj = json_object_object_get(data, "channel_id");
    objstr = json_object_get_string(obj);
    success = snowflake_from_string(objstr, &message->reference->channel_id);

    if (!success){
        return false;
    }

    obj = json_object_object_get(data, "guild_id");
    objstr = json_object_get_string(obj);
    success = snowflake_from_string(objstr, &message->reference->guild_id);

    if (!success){
        return false;
    }

    obj = json_object_object_get(data, "fail_if_not_exists");
    message->reference->fail_if_not_exists = json_object_get_boolean(obj);

    return success;
}

static bool construct_message_mentions(discord_message *message, json_object *data){
    if (message->mentions){
        list_empty(message->mentions);
    }
    else {
        message->mentions = list_init();

        if (!message->mentions){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mentions() - mentions initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *obj = json_object_array_get_idx(data, index);
        const discord_user *user = state_set_user(message->state, obj);

        if (!user){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mentions() - state_set_user call failed for %s\n",
                __FILE__,
                json_object_to_json_string(obj)
            );

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*user);
        item.data_copy = user;

        success = list_append(message->mentions, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mentions() - list_append call failed\n",
                __FILE__
            );

            break;
        }
    }

    return success;
}

static bool construct_message_mention_roles(discord_message *message, json_object *data){
    if (message->mention_roles){
        list_empty(message->mention_roles);
    }
    else {
        message->mention_roles = list_init();

        if (!message->mention_roles){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mention_roles() - mention_roles initialization failed\n",
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
                "[%s] construct_message_mention_roles() - snowflake_from_string call failed for %s\n",
                __FILE__,
                json_object_to_json_string(obj)
            );

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_UINT;
        item.size = sizeof(id);
        item.data_copy = &id;

        success = list_append(message->mention_roles, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mention_roles() - list_append call failed\n",
                __FILE__
            );

            break;
        }
    }

    return success;
}

static bool construct_message_mention_channels(discord_message *message, json_object *data){
    if (message->mention_channels){
        list_empty(message->mention_channels);
    }
    else {
        message->mention_channels = list_init();

        if (!message->mention_channels){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mention_channels() - mention_channels initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    discord_message_channel_mention *cmention = NULL;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *valueobj = json_object_array_get_idx(data, index);

        cmention = calloc(1, sizeof(*cmention));

        if (!cmention){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mention_channels() - cmention alloc failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        json_object *obj = json_object_object_get(valueobj, "id");
        const char *objstr = json_object_get_string(obj);
        success = snowflake_from_string(objstr, &cmention->id);

        if (!success){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] construct_message_mention_channels() - snowflake_from_string call failed for id\n",
                __FILE__
            );

            break;
        }

        obj = json_object_object_get(valueobj, "guild_id");
        objstr = json_object_get_string(obj);
        success = snowflake_from_string(objstr, &cmention->guild_id);

        if (!success){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] construct_message_mention_channels() - snowflake_from_string call failed for guild_id\n",
                __FILE__
            );

            break;
        }

        obj = json_object_object_get(valueobj, "type");
        cmention->type = json_object_get_int(obj);

        obj = json_object_object_get(valueobj, "name");
        cmention->name = json_object_get_string(obj);

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*cmention);
        item.data = cmention;

        success = list_append(message->mention_roles, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_mention_roles() - list_append call failed\n",
                __FILE__
            );

            break;
        }
    }

    if (!success){
        free(cmention);
    }

    return success;
}

static bool construct_message_attachments(discord_message *message, json_object *data){
    if (message->attachments){
        list_empty(message->attachments);
    }
    else {
        message->attachments = list_init();

        if (!message->attachments){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_attachments() - attachments initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *obj = json_object_array_get_idx(data, index);
        discord_attachment *attachment = attachment_init(message->state, obj);

        if (!attachment){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_attachments() - attachment initialization failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*attachment);
        item.data = attachment;

        success = list_append(message->attachments, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_attachments() - list_append call failed\n",
                __FILE__
            );

            attachment_free(attachment);

            break;
        }
    }

    return success;
}

static bool construct_message_reactions(discord_message *message, json_object *data){
    if (message->reactions){
        list_empty(message->reactions);
    }
    else {
        message->reactions = list_init();

        if (!message->reactions){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_reactions() - reactions initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *obj = json_object_array_get_idx(data, index);
        discord_reaction *reaction = reaction_init(message->state, obj);

        if (!reaction){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_reactions() - reaction initialization failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*reaction);
        item.data = reaction;
        item.generic_free = reaction_free;

        success = list_append(message->reactions, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message_reactions() - list_append call failed\n",
                __FILE__
            );

            reaction_free(reaction);

            break;
        }
    }

    return success;
}

static bool construct_message(discord_message *message){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(message->raw_object);
    struct json_object_iterator end = json_object_iter_end(message->raw_object);

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

            success = snowflake_from_string(objstr, &message->id);
        }
        else if (!strcmp(key, "channel_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &message->channel_id);
        }
        else if (!strcmp(key, "guild_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &message->guild_id);
        }
        else if (!strcmp(key, "author")){
            message->author = state_set_user(message->state, valueobj);

            success = message->author;
        }
        else if (!strcmp(key, "member")){
            if (message->member){
                member_free(message->member);

                message->member = NULL;
            }

            message->member = member_init(message->state, valueobj);

            success = message->member;
        }
        else if (!strcmp(key, "content")){
            message->content = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "timestamp")){
            /* i'd like to convert this to a time_t */
            message->timestamp = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "edited_timestamp")){
            /* same as above */
            message->edited_timestamp = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "tts")){
            message->tts = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "mention_everyone")){
            message->mention_everyone = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "mentions")){
            success = construct_message_mentions(message, valueobj);
        }
        else if (!strcmp(key, "mention_roles")){
            success = construct_message_mention_roles(message, valueobj);
        }
        else if (!strcmp(key, "mention_channels")){
            success = construct_message_mention_channels(message, valueobj);
        }
        else if (!strcmp(key, "attachments")){
            success = construct_message_attachments(message, valueobj);
        }
        else if (!strcmp(key, "embeds")){
            if (message->embeds){
                list_free(message->embeds);
            }

            message->embeds = embed_list_from_json_array(message->state, valueobj);

            success = message->embeds;
        }
        else if (!strcmp(key, "reactions")){
            success = construct_message_reactions(message, valueobj);
        }
        else if (!strcmp(key, "nonce")){
            message->nonce = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "pinned")){
            message->pinned = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "webhook_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &message->webhook_id);
        }
        else if (!strcmp(key, "type")){
            message->type = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "activity")){
            if (message->activity){
                free(message->activity);

                message->activity = NULL;
            }

            success = construct_message_activity(message, valueobj);
        }
        else if (!strcmp(key, "application")){
            message->application = application_init(message->state, valueobj);

            success = message->application;
        }
        else if (!strcmp(key, "application_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &message->application_id);
        }
        else if (!strcmp(key, "message_reference")){
            if (message->reference){
                free(message->reference);

                message->reference = NULL;
            }

            success = construct_message_reference(message, valueobj);
        }
        else if (!strcmp(key, "flags")){
            message->flags = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "referenced_message")){
            message->referenced_message = state_set_message(
                message->state,
                valueobj,
                false
            );

            success = message->referenced_message;
        }
        else if (!strcmp(key, "interaction")){
            // interaction.c ???
            log_write(
                logger,
                LOG_RAW,
                "interaction content\n%s\n",
                json_object_to_json_string_ext(valueobj, JSON_C_TO_STRING_PRETTY)
            );
        }
        else if (!strcmp(key, "thread")){
            // state_set_channel(message->state, data, false); --- BOOKMARK ---
            log_write(
                logger,
                LOG_RAW,
                "thread content\n%s\n",
                json_object_to_json_string_ext(valueobj, JSON_C_TO_STRING_PRETTY)
            );
        }
        else if (!strcmp(key, "components")){
            // component.c ???
            log_write(
                logger,
                LOG_RAW,
                "components content\n%s\n",
                json_object_to_json_string_ext(valueobj, JSON_C_TO_STRING_PRETTY)
            );
        }
        else if (!strcmp(key, "sticker_items")){
            // sticker.c
            log_write(
                logger,
                LOG_RAW,
                "sticker items content\n%s\n",
                json_object_to_json_string_ext(valueobj, JSON_C_TO_STRING_PRETTY)
            );
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_message() - failed to set %s with value: %s\n",
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

discord_message *message_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] message_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] message_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_message *message = calloc(1, sizeof(*message));

    if (!message){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_init() - message alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    message->state = state;
    message->raw_object = json_object_get(data);

    if (!construct_message(message)){
        message_free(message);

        message = NULL;
    }

    return message;
}

bool message_update(discord_message *message, json_object *data){
    if (!message){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] message_update() - message is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] message_update() - data is NULL\n",
            __FILE__
        );

        return false;
    }

    if (!json_merge_objects(data, message->raw_object)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_update() - json_merge_objects call failed\n",
            __FILE__
        );

        return false;
    }

    return construct_message(message);
}

/* API calls */
bool message_delete(const discord_message *message, const char *reason){
    if (!message){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_delete() - message is NULL\n",
            __FILE__
        );

        return false;
    }

    discord_http_response *res = discord_http_delete_message(
        message->state->http,
        message->channel_id,
        message->id,
        reason
    );

    if (!res){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_delete() - discord_http_delete_message call failed\n",
            __FILE__
        );

        return false;
    }

    bool success = res->status == 204;

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_delete() - API request failed: %s\n",
            __FILE__,
            json_object_to_json_string(res->data)
        );
    }

    discord_http_response_free(res);

    return success;
}

void message_free(void *messageptr){
    discord_message *message = messageptr;

    if (!message){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] message_free() - message is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(message->raw_object);

    /* --- BOOKMARK --- add guild object, guild will have ownership of member */
    member_free(message->member);

    list_free(message->mentions);
    list_free(message->mention_roles);
    list_free(message->mention_channels);
    list_free(message->attachments);
    list_free(message->embeds);
    list_free(message->reactions);
    list_free(message->components);
    list_free(message->sticker_items);

    application_free(message->application);

    free(message->activity);
    free(message->reference);
    free(message);
}

/* message reply functions */

static bool set_reply_json_content(json_object *replyobj, const char *content){
    if (!content){
        return true;
    }

    json_object *obj = json_object_new_string(content);
    bool success = !json_object_object_add(replyobj, "content", obj);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_reply_json_content() - json_object_object_add call failed\n",
            __FILE__
        );
    }

    return success;
}

static bool set_reply_json_misc(json_object *replyobj, bool tts, int flags){
    json_object *obj = json_object_new_boolean(tts);
    bool success = !json_object_object_add(replyobj, "tts", obj);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_reply_json_misc() - json_object_object_add call failed for tts\n",
            __FILE__
        );

        return false;
    }

    obj = json_object_new_int(flags);
    success = !json_object_object_add(replyobj, "flags", obj);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_reply_json_misc() - json_object_object_add call failed for flags\n",
            __FILE__
        );
    }

    return success;
}

static bool set_reply_json_embeds(json_object *replyobj, const discord_embed *embed, const list *embeds){
    if (!embed && !embeds){
        return true;
    }

    json_object *embedsobj = json_object_new_array();

    if (!embedsobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_reply_json_embeds() - embeds object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(replyobj, "embeds", embedsobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_reply_json_embeds() - json_object_object_add call failed for embeds\n",
            __FILE__
        );

        json_object_put(embedsobj);

        return false;
    }

    if (embed){
        json_object *copy = json_object_get(embed->raw_object);

        if (!copy){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_reply_json_embeds() - json_object_get call failed for embed\n",
                __FILE__
            );

            return false;
        }

        if (json_object_array_add(embedsobj, copy)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_reply_json_embeds() - json_object_array_add call failed for embed\n",
                __FILE__
            );

            json_object_put(copy);

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < list_get_length(embeds); ++index){
        embed = list_get_generic(embeds, index);
        json_object *copy = json_object_get(embed->raw_object);

        if (!copy){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_reply_json_embeds() - json_object_get_call failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        if (json_object_array_add(embedsobj, embed->raw_object)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_reply_json_embeds() - json_object_array_add call failed\n",
                __FILE__
            );

            json_object_put(copy);

            success = false;

            break;
        }
    }

    return success;
}

/* --- BOOKMARK --- convert this to the constructor format */
json_object *message_reply_to_json(const discord_message_reply *reply){
    if (!reply){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_reply_to_json() - reply is NULL\n",
            __FILE__
        );

        return NULL;
    }

    json_object *replyobj = json_object_new_object();

    if (!replyobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] message_reply_to_json() - reply object initialization failed\n",
            __FILE__
        );

        return NULL;
    }

    if (!set_reply_json_content(replyobj, reply->content)){
        json_object_put(replyobj);

        return NULL;
    }

    if (!set_reply_json_misc(replyobj, reply->tts, reply->flags)){
        json_object_put(replyobj);

        return NULL;
    }

    if (!set_reply_json_embeds(replyobj, reply->embed, reply->embeds)){
        json_object_put(replyobj);

        return NULL;
    }

    /* support the rest */

    return replyobj;
}
