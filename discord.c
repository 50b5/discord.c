#include "discord.h"

static const logctx *logger = NULL;

static bool set_application_information(discord *client){
    discord_http_response *res = discord_http_get_current_application_information(
        client->state->http
    );

    if (!res){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_application_information() - discord_http_get_current_application_information call failed\n",
            __FILE__
        );

        return false;
    }
    else if (res->status != 200){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] set_application_information() - request failed: %s\n",
            __FILE__,
            json_object_to_json_string(res->data)
        );
    }
    else {
        client->application = application_init(client->state, res->data);

        if (!client->application){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] set_application_information() - application_init call failed\n",
                __FILE__
            );
        }
    }

    discord_http_response_free(res);

    return client->application ? true : false;
}

discord *discord_init(const char *token, const discord_options *opts){
    logger = opts ? opts->log : NULL;

    if (!token){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_init() - a bot token is required\n",
            __FILE__
        );

        return NULL;
    }

    discord *client = calloc(1, sizeof(*client));

    if (!client){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_init() - calloc for client failed\n",
            __FILE__
        );

        return NULL;
    }

    discord_state_options sopts = {0};
    discord_gateway_options gopts = {0};

    if (opts){
        sopts.log = opts->log;
        sopts.intent = opts->intent;
        sopts.max_messages = opts->max_messages;

        gopts.compress = opts->compress;
        gopts.large_threshold = opts->large_threshold;
        gopts.events = opts->events;
    }

    client->state = state_init(token, &sopts);

    if (!client->state){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_init() - state_init call failed\n",
            __FILE__
        );

        discord_free(client);

        return NULL;
    }
    else if (opts && opts->presence){
        if (!state_set_presence(client->state, opts->presence)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] discord_init() - state_set_presence call failed\n",
                __FILE__
            );

            discord_free(client);

            return NULL;
        }
    }

    client->state->event_context = client;
    client->state->user_pointer = (void *)&client->user;

    if (!set_application_information(client)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_init() - set_application_information call failed\n",
            __FILE__
        );

        discord_free(client);

        return NULL;
    }

    client->gateway = gateway_init(client->state, &gopts);

    if (!client->gateway){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_init() - gateway initialization failed\n",
            __FILE__
        );

        discord_free(client);

        return NULL;
    }

    return client;
}

bool discord_connect_gateway(discord *client){
    if (!client){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_connect_gateway() - client is NULL\n",
            __FILE__
        );

        return false;
    }

    if (!gateway_connect(client->gateway)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_connect_gateway() - gateway_connect call failed\n",
            __FILE__
        );

        return false;
    }

    return gateway_run_loop(client->gateway);
}

void discord_disconnect_gateway(discord *client){
    if (!client){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_disconnect_gateway() - client is NULL\n",
            __FILE__
        );

        return;
    }

    gateway_disconnect(client->gateway);
}

bool discord_set_presence(discord *client, const discord_presence *presence){
    if (!client){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_set_presence() - client is NULL\n",
            __FILE__
        );

        return false;
    }

    bool success = state_set_presence(
        client->state,
        presence
    );

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_set_presence() - state_set_presence call failed\n",
            __FILE__
        );

        return false;
    }

    success = gateway_send(
        client->gateway,
        GATEWAY_OP_PRESENCE_UPDATE,
        state_get_presence(client->state)
    );

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_set_presence() - gateway_send call failed\n",
            __FILE__
        );
    }

    return success;
}

bool discord_modify_presence(discord *client, const time_t *since, const list *activities, const char *status, const bool *afk){
    if (!client){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_set_presence() - client is NULL\n",
            __FILE__
        );

        return false;
    }

    bool success = true;

    if (since){
        success = state_set_presence_since(client->state, *since);

        if (!success){
            return false;
        }
    }

    if (activities){
        success = state_set_presence_activities(client->state, activities);

        if (!success){
            return false;
        }
    }

    if (status){
        success = state_set_presence_status(client->state, status);

        if (!success){
            return false;
        }
    }

    if (afk){
        success = state_set_presence_afk(client->state, *afk);

        if (!success){
            return false;
        }
    }

    success = gateway_send(
        client->gateway,
        GATEWAY_OP_PRESENCE_UPDATE,
        state_get_presence(client->state)
    );

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_modify_presence() - gateway_send call failed\n",
            __FILE__
        );
    }

    return success;
}

const discord_user *discord_get_user(discord *client, snowflake id, bool fetch){
    if (!client){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] discord_get_user() - client is NULL\n",
            __FILE__
        );

        return NULL;
    }

    const discord_user *user = NULL;

    if (fetch){
        discord_http_response *res = discord_http_get_user(client->state->http, id);

        if (!res){
            if (!client->state->http->ratelimited){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] discord_get_user() - discord_http_get_user call failed\n",
                    __FILE__
                );
            }

            return NULL;
        }
        else if (res->status != 200){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] discord_get_user() - request failed: %s\n",
                __FILE__,
                json_object_to_json_string(res->data)
            );

            discord_http_response_free(res);

            return NULL;
        }

        user = state_set_user(client->state, res->data);

        discord_http_response_free(res);
    }
    else {
        user = state_get_user(client->state, id);
    }

    return user;
}

bool discord_send_message(discord *client, snowflake channelid, const discord_message_reply *message){
    if (!client){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] discord_send_message() - client is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!message){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] discord_send_message() - message is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!message->content && !message->embed && !message->embeds && !message->sticker_ids){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] discord_send_message() - one of content, file, embeds, sticker_ids required\n",
            __FILE__
        );

        return true;
    }

    json_object *data = message_reply_to_json(message);

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] discord_send_message() - message_reply_to_json call failed\n",
            __FILE__
        );

        return false;
    }

    discord_http_response *res = discord_http_create_message(client->state->http, channelid, data);

    json_object_put(data);

    bool success = true;

    if (!res){
        if (!client->state->http->ratelimited){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] discord_send_message() - discord_http_create_message call failed\n",
                __FILE__
            );

            success = false;
        }
    }
    else if (res->status != 200){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] discord_send_message() - request failed: %s\n",
            __FILE__,
            json_object_to_json_string(res->data)
        );
    }

    discord_http_response_free(res);

    return success;
}

void discord_free(discord *client){
    if (!client){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] discord_free() - client is NULL\n",
            __FILE__
        );

        return;
    }

    state_free(client->state);
    gateway_free(client->gateway);

    application_free(client->application);

    free(client);
}
