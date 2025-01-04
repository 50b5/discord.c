#include "gateway.h"

static const logctx *logger = NULL;

typedef struct gateway_receive_buffer {
    char *data;
    size_t length;
} gateway_receive_buffer;

static int handle_gateway_event(struct lws *, enum lws_callback_reasons, void *, void *, size_t);

static const struct lws_protocols lwsprotocols[] = {
    {
        "handle_gateway_event",
        &handle_gateway_event,
        sizeof(discord_gateway),
        4096,
        0,
        NULL,
        0
    },

    LWS_PROTOCOL_LIST_TERM
};

static bool is_rate_limited(discord_gateway *gateway){
    unsigned long now = lws_now_secs();
    unsigned long diff = now - gateway->last_sent;
    unsigned long remaining = 0;

    if (diff < DISCORD_GATEWAY_RATE_LIMIT_INTERVAL){
        remaining = DISCORD_GATEWAY_RATE_LIMIT_INTERVAL - diff;
    }

    log_write(
        logger,
        LOG_DEBUG,
        "[%s] is_rate_limited() - %d requests so far -- last sent %ld seconds ago -- %ld seconds remaining till reset\n",
        __FILE__,
        gateway->sent_count,
        gateway->last_sent ? diff : 0,
        remaining
    );

    if (!remaining){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] is_rate_limited() - gateway connection is out of rate limit window -- resetting sent_count to 0\n",
            __FILE__
        );

        gateway->sent_count = 0;

        return false;
    }

    log_write(
        logger,
        LOG_DEBUG,
        "[%s] is_rate_limited() - gateway connection is within rate limit window -- checking if sent_count >= RATE_LIMIT_COUNT (%d >= %d)\n",
        __FILE__,
        gateway->sent_count,
        DISCORD_GATEWAY_RATE_LIMIT_COUNT
    );

    bool ratelimited = gateway->sent_count >= DISCORD_GATEWAY_RATE_LIMIT_COUNT;

    if (ratelimited){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] is_rate_limited() - gateway connection is rate limited\n",
            __FILE__
        );
    }
    else {
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] is_rate_limited() - gateway connection is not rate limited\n",
            __FILE__
        );
    }

    return ratelimited;
}

static void cancel_gateway_heartbeating(discord_gateway *gateway){
    gateway->awaiting_heartbeat_ack = false;

    lws_set_timer_usecs(gateway->wsi, LWS_SET_TIMER_USEC_CANCEL);
}

static bool send_gateway_heartbeat(discord_gateway *gateway){
    if (!gateway->connected){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] send_gateway_heartbeat() - tried to send heartbeat but gateway is not connected\n",
            __FILE__
        );

        return false;
    }

    if (gateway->awaiting_heartbeat_ack){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] send_gateway_heartbeat() - gateway server did not acknowledge last HEARTBEAT\n",
            __FILE__
        );

        gateway->reconnect = true;
        gateway->resume = true;

        gateway_disconnect(gateway);

        return true;
    }

    json_object *sequenceobj = NULL;

    if (gateway->last_sequence){
        sequenceobj = json_object_new_int(gateway->last_sequence);

        if (!sequenceobj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] send_gateway_heartbeat() - sequence object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = gateway_send(gateway, GATEWAY_OP_HEARTBEAT, sequenceobj);

    json_object_put(sequenceobj);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_heartbeat() - gateway_send call failed\n",
            __FILE__
        );

        return false;
    }

    gateway->awaiting_heartbeat_ack = true;

    return true;
}

static bool send_gateway_identify(discord_gateway *gateway){
    const char *datafmt = "{"
                          "\"token\": \"%s\", "
                          "\"compress\": %s, "
                          "\"large_threshold\": %d, "
                          "\"intents\": %d, "
                          "\"presence\": %s, "
                          "\"properties\": {"
                              "\"$os\": \"%s\", "
                              "\"$browser\": \"%s\", "
                              "\"$device\": \"%s\""
                          "}"
                          "}";

    char *datastr = string_create(
        datafmt,
        gateway->state->token,
        gateway->compress ? "true" : "false",
        gateway->large_threshold,
        gateway->state->intent,
        state_get_presence_string(gateway->state),
        DISCORD_LIBRARY_OS,
        DISCORD_LIBRARY_NAME,
        DISCORD_LIBRARY_NAME
    );

    if (!datastr){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_identify() - string_create call failed\n",
            __FILE__
        );

        return false;
    }

    json_object *data = json_tokener_parse(datastr);

    free(datastr);

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_identify() - json_tokener_parse call failed\n",
            __FILE__
        );

        return false;
    }

    bool success = gateway_send(gateway, GATEWAY_OP_IDENTIFY, data);

    json_object_put(data);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_identify() - gateway_send call failed\n",
            __FILE__
        );
    }

    return success;
}

static bool send_gateway_resume(discord_gateway *gateway){
    const char *datafmt = "{"
                          "\"token\": \"%s\", "
                          "\"session_id\": \"%s\", "
                          "\"seq\": %d"
                          "}";

    char *datastr = string_create(
        datafmt,
        gateway->state->token,
        gateway->session_id,
        gateway->last_sequence
    );

    if (!datastr){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_resume() - string_create call failed\n",
            __FILE__
        );

        return false;
    }

    json_object *data = json_tokener_parse(datastr);

    free(datastr);

    if (!data){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_resume() - json_tokener_parse call failed\n",
            __FILE__
        );

        return false;
    }

    bool success = gateway_send(gateway, GATEWAY_OP_RESUME, data);

    json_object_put(data);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] send_gateway_resume() - gateway_send call failed\n",
            __FILE__
        );
    }

    return success;
}

static discord_gateway_event get_gateway_event_callback(discord_gateway *gateway, const char *name){
    discord_gateway_event event = NULL;

    for (size_t index = 0; gateway->events[index].name; ++index){
        if (!strcmp(gateway->events[index].name, name)){
            event = gateway->events[index].event;

            break;
        }
    }

    return event;
}

static bool handle_gateway_dispatch(discord_gateway *gateway, const char *name, json_object *data){
    log_write(
        logger,
        LOG_DEBUG,
        "[%s] handle_gateway_dispatch() - gateway server dispatched event %s\n",
        __FILE__,
        name
    );

    if (!gateway->events){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_dispatch() - no event callbacks have been set\n",
            __FILE__
        );

        return true;
    }

    const void *eventdata = NULL;

    if (!strcmp(name, "READY")){
        const discord_user *user = state_set_user(
            gateway->state,
            json_object_object_get(data, "user")
        );

        if (!user){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_dispatch() - state_set_user call failed\n",
                __FILE__
            );

            return false;
        }

        gateway->state->user = user;
        *gateway->state->user_pointer = user;

        const char *sessionid = json_object_get_string(json_object_object_get(data, "session_id"));

        if (!sessionid){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_dispatch() - missing session_id from data\n",
                __FILE__
            );

            return false;
        }

        string_copy(sessionid, gateway->session_id, sizeof(gateway->session_id));

        eventdata = gateway->state->user;
    }
    else if (!strcmp(name, "RESUMED")){
        gateway->resume = false;

        eventdata = gateway->state->user;
    }
    else if (!strcmp(name, "GUILD_CREATE")){
        /* set guild up for cache */
    }
    else if (!strcmp(name, "MESSAGE_CREATE")){
        const discord_message *message = state_set_message(gateway->state, data, false);

        if (!message){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_dispatch() - state_set_message call failed\n",
                __FILE__
            );

            return false;
        }

        eventdata = message;
    }
    else if (!strcmp(name, "MESSAGE_UPDATE")){
        const discord_message *message = state_set_message(gateway->state, data, true);

        if (!message){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_dispatch() - state_set_message call failed\n",
                __FILE__
            );

            return false;
        }

        eventdata = message;
    }
    else if (!strcmp(name, "MESSAGE_DELETE")){
        const char *idstr = json_object_get_string(
            json_object_object_get(data, "id")
        );

        if (!idstr){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] id_from_message_object() - failed to get id from data: %s\n",
                __FILE__,
                json_object_to_json_string(data)
            );

            return false;
        }

        static snowflake id = 0;
        bool success = snowflake_from_string(idstr, &id);

        if (!success){
            log_write(
                logger,
                LOG_WARNING,
                "[%s] id_from_message_object() - snowflake_from_string call failed for id: %s\n",
                __FILE__,
                idstr
            );

            return false;
        }

        eventdata = &id;
    }

    discord_gateway_event event = get_gateway_event_callback(gateway, name);

    if (!event){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_dispatch() - no event callback set for event %s\n",
            __FILE__,
            name
        );

        return true;
    }

    return event(gateway->state->event_context, eventdata);
}

static bool handle_gateway_payload(discord_gateway *gateway){
    json_object *payload = json_tokener_parse(gateway->buffer->data);

    if (!payload){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] handle_gateway_payload() - json_tokener_parse call failed on data %s\n",
            __FILE__,
            gateway->buffer->data
        );

        return false;
    }

    int op = json_object_get_int(json_object_object_get(payload, "op"));
    json_object *d = json_object_object_get(payload, "d");
    int s = json_object_get_int(json_object_object_get(payload, "s"));
    const char *t = json_object_get_string(json_object_object_get(payload, "t"));

    if (op != GATEWAY_OP_HELLO && !gateway->connected){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_payload() - attempted call but gateway->connected is false\n",
            __FILE__
        );

        return false;
    }

    bool success = true;

    switch (op){
    case GATEWAY_OP_DISPATCH:
        gateway->last_sequence = s;

        success = handle_gateway_dispatch(gateway, t, d);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_payload() - handle_gateway_dispatch call failed\n",
                __FILE__
            );
        }

        break;
    case GATEWAY_OP_HEARTBEAT:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_payload() - gateway server sent HEARTBEAT\n",
            __FILE__
        );

        cancel_gateway_heartbeating(gateway);

        lws_set_timer_usecs(gateway->wsi, 0);

        break;
    case GATEWAY_OP_RECONNECT:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_payload() - gateway server sent RECONNECT\n",
            __FILE__
        );

        gateway->reconnect = true;
        gateway->resume = true;

        gateway_disconnect(gateway);

        break;
    case GATEWAY_OP_INVALID_SESSION:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_payload() - gateway server sent INVALID SESSION\n",
            __FILE__
        );

        gateway->reconnect = true;
        gateway->resume = json_object_get_boolean(d);

        gateway_disconnect(gateway);

        break;
    case GATEWAY_OP_HELLO:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_payload() - gateway server sent HELLO\n",
            __FILE__
        );

        gateway->connected = true;
        gateway->heartbeat_interval_us = json_object_get_int(
            json_object_object_get(d, "heartbeat_interval")
        );

        if (gateway->heartbeat_interval_us < 10000){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_payload() - gateway server sent <10 second HEARTBEAT interval\n",
                __FILE__
            );

            success = false;

            break;
        }

        gateway->heartbeat_interval_us *= 1000;

        lws_set_timer_usecs(
            gateway->wsi,
            gateway->heartbeat_interval_us * DISCORD_GATEWAY_HEARTBEAT_JITTER
        );

        if (gateway->resume){
            log_write(
                logger,
                LOG_DEBUG,
                "[%s] handle_gateway_payload() - sending RESUME to gateway server\n",
                __FILE__
            );

            success = send_gateway_resume(gateway);

            if (!success){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] handle_gateway_payload() - send_gateway_resume call failed\n",
                    __FILE__
                );
            }
        }
        else {
            log_write(
                logger,
                LOG_DEBUG,
                "[%s] handle_gateway_payload() - sending IDENTIFY to gateway server\n",
                __FILE__
            );

            success = send_gateway_identify(gateway);

            if (!success){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] handle_gateway_payload() - send_gateway_identify call failed\n",
                    __FILE__
                );
            }
        }

        break;
    case GATEWAY_OP_HEARTBEAT_ACK:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_payload() - gateway server acknowledged client HEARTBEAT\n",
            __FILE__
        );

        gateway->awaiting_heartbeat_ack = false;

        lws_validity_confirmed(gateway->wsi);

        break;
    default:
        log_write(
            logger,
            LOG_WARNING,
            "[%s] handle_gateway_payload() - gateway server sent unknown op %d\n",
            __FILE__,
            op
        );
    }

    json_object_put(payload);

    return success;
}

static bool handle_gateway_writable(discord_gateway *gateway, struct lws *wsi){
    size_t queuelen = list_get_length(gateway->queue);

    if (!queuelen){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_writable() - nothing in queue -- returning to event loop\n",
            __FILE__
        );

        return true;
    }

    list_item payload = {0};

    list_pop(gateway->queue, 0, &payload);

    unsigned char *data = payload.data;

    size_t datalen = strlen((char *)(data + LWS_PRE));
    int ret = lws_write(wsi, data + LWS_PRE, datalen, LWS_WRITE_TEXT);

    free(data);

    if (ret < 0){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] handle_gateway_writable() - write to gateway socket failed (%d)\n",
            __FILE__,
            ret
        );

        return false;
    }
    else if ((size_t)ret < datalen){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] handle_gateway_writable() - write to gateway socket unfinished (%d out of %ld)\n",
            __FILE__,
            ret,
            datalen
        );

        return false;
    }

    return true;
}

static bool handle_gateway_receive(discord_gateway *gateway, struct lws *wsi, void *data, size_t datalen){
    bool first_frag = lws_is_first_fragment(wsi);
    bool last_frag = lws_is_final_fragment(wsi);

    if (first_frag){
        gateway->buffer->length = 0;
    }

    char *tmp = realloc(gateway->buffer->data, gateway->buffer->length + datalen + 1);

    if (!tmp){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] handle_gateway_event() - buffer data object realloc failed\n",
            __FILE__
        );

        return -1;
    }

    memcpy(tmp + gateway->buffer->length, data, datalen);

    gateway->buffer->data = tmp;
    gateway->buffer->length += datalen;
    gateway->buffer->data[gateway->buffer->length] = '\0';

    return last_frag ? handle_gateway_payload(gateway) : true;
}

int handle_gateway_event(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *data, size_t datalen){
    if (user){
        /* ignored for now */
    }

    discord_gateway *gateway = lws_context_user(lws_get_context(wsi));

    bool closeconn = false;
    bool success = true;

    switch (reason){
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - LWS_CALLBACK_CLIENT_CONNECTION_ERROR event triggered\n",
            __FILE__
        );

        success = false;

        break;
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - handshake with the gateway server completed successfully\n",
            __FILE__
        );

        break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - ready to receive data from the gateway server\n",
            __FILE__
        );

        success = handle_gateway_receive(gateway, wsi, data, datalen);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_event() - handle_gateway_receive call failed\n",
                __FILE__
            );
        }

        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - ready to send data to the gateway server\n",
            __FILE__
        );

        if (gateway->connected){
            success = handle_gateway_writable(gateway, wsi);

            if (!success){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] handle_gateway_event() - handle_gateway_writable call failed\n",
                    __FILE__
                );
            }

            break;
        }

        closeconn = true;

        break;
    case LWS_CALLBACK_TIMER:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - LWS_CALLBACK_TIMER event triggered\n",
            __FILE__
        );

        if (!gateway->connected){
            closeconn = true;

            break;
        }

        success = send_gateway_heartbeat(gateway);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_event() - send_gateway_heartbeat call failed\n",
                __FILE__
            );

            break;
        }

        lws_set_timer_usecs(wsi, gateway->heartbeat_interval_us);

        break;
    case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - gateway server sent PONG\n",
            __FILE__
        );

        lws_validity_confirmed(wsi);

        break;
    case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - gateway server initiated close\n",
            __FILE__
        );

        break;
    case LWS_CALLBACK_CLOSED:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - gateway server closed connection\n",
            __FILE__
        );

        break;
    case LWS_CALLBACK_CLIENT_CLOSED:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - closed connection to gateway server\n",
            __FILE__
        );

        if (gateway->connected){
            log_write(
                logger,
                LOG_DEBUG,
                "[%s] handle_gateway_event() - unexpected disconnect -- reconnecting\n",
                __FILE__
            );

            gateway->reconnect = true;
            gateway->resume = true;

            gateway_disconnect(gateway);
        }

        break;
    case LWS_CALLBACK_WSI_DESTROY:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - LWS_CALLBACK_WSI_DESTROY event triggered\n",
            __FILE__
        );

        closeconn = true;

        if (!gateway->reconnect){
            log_write(
                logger,
                LOG_DEBUG,
                "[%s] handle_gateway_event() - gateway->reconnect not set\n",
                __FILE__
            );

            gateway->running = false;

            break;
        }

        /* NOTE need actual logic here */
        sleep(3);

        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - attempting to reconnect to gateway server (resume: %s)\n",
            __FILE__,
            gateway->resume ? "true" : "false"
        );

        if (!gateway->resume){
            gateway->session_id[0] = '\0';
            gateway->last_sequence = 0;
        }

        gateway->last_sent = 0;
        gateway->sent_count = 0;

        success = gateway_connect(gateway);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_event() - gateway_connect call failed\n",
                __FILE__
            );
        }

        break;
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] handle_gateway_event() - protocol destroyed -- shutting down event loop\n",
            __FILE__
        );

        closeconn = true;

        gateway->running = false;

        break;
    default:
        if (DISCORD_GATEWAY_LWS_LOG_LEVEL){
            log_write(
                logger,
                LOG_DEBUG,
                "[%s] handle_gateway_event() - unhandled reason %d\n",
                __FILE__,
                reason
            );
        }

        break;
    }

    if (!success){
        if (data && datalen){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_event() - encountered error while handling an event (%.*s)\n",
                __FILE__,
                datalen,
                data
            );
        }
        else {
            log_write(
                logger,
                LOG_ERROR,
                "[%s] handle_gateway_event() - encountered error while handling an event\n",
                __FILE__
            );
        }

        closeconn = true;

        gateway->running = false;
    }

    return closeconn ? -1 : 0;
}

static bool set_gateway_endpoint(discord_gateway *gateway){
    discord_http_response *response = discord_http_get_bot_gateway(gateway->state->http);

    if (!response){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_gateway_endpoint() - discord_http_get_bot_gateway call failed\n",
            __FILE__
        );

        return false;
    }
    else if (response->status != 200){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_gateway_endpoint() - API request failed: %s\n",
            __FILE__,
            json_object_to_json_string(response->data)
        );

        discord_http_response_free(response);

        return false;
    }

    json_object *sessiondata = json_object_object_get(
        response->data,
        "session_start_limit"
    );

    gateway->total_session_starts = json_object_get_int(
        json_object_object_get(sessiondata, "total")
    );

    gateway->remaining_session_starts = json_object_get_int(
        json_object_object_get(sessiondata, "remaining")
    );

    gateway->reset_after = json_object_get_int(
        json_object_object_get(sessiondata, "reset_after")
    );

    gateway->max_concurrency = json_object_get_int(
        json_object_object_get(sessiondata, "max_concurrency")
    );

    gateway->shards = json_object_get_int(
        json_object_object_get(response->data, "shards")
    );

    if (gateway->endpoint){
        discord_http_response_free(response);

        return true;
    }

    const char *url = json_object_get_string(
        json_object_object_get(response->data, "url")
    );

    char *endpoint = string_create(
        "%s/?v=%d&encoding=%s",
        url,
        DISCORD_GATEWAY_VERSION,
        DISCORD_GATEWAY_ENCODING
    );

    discord_http_response_free(response);

    if (!endpoint){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] set_gateway_endpoint() - endpoint string alloc failed\n",
            __FILE__
        );

        return false;
    }

    gateway->endpoint = endpoint;

    return true;
}

discord_gateway *gateway_init(discord_state *state, const discord_gateway_options *opts){
    if (!state){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    lws_set_log_level(DISCORD_GATEWAY_LWS_LOG_LEVEL, NULL);

    discord_gateway *gateway = calloc(1, sizeof(*gateway));

    if (!gateway){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_init() - alloc for gateway object failed\n",
            __FILE__
        );

        return NULL;
    }

    gateway->state = state;
    gateway->version = DISCORD_GATEWAY_VERSION;

    gateway->running = true;

    if (opts){
        gateway->compress = opts->compress;
        gateway->events = opts->events;
    }

    gateway->queue = list_init();

    if (!gateway->queue){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_init() - payload queue initialization failed\n",
            __FILE__
        );

        gateway_free(gateway);

        return NULL;
    }

    gateway->buffer = calloc(1, sizeof(*gateway->buffer));

    if (!gateway->buffer){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_init() - buffer initialization failed\n",
            __FILE__
        );

        gateway_free(gateway);

        return NULL;
    }

    struct lws_context_creation_info ctxinfo = {0};
    ctxinfo.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    ctxinfo.port = CONTEXT_PORT_NO_LISTEN;
    ctxinfo.protocols = lwsprotocols;
    ctxinfo.user = gateway;

    gateway->context = lws_create_context(&ctxinfo);

    if (!gateway->context){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_init() - lws_create_context call failed\n",
            __FILE__
        );

        gateway_free(gateway);

        return NULL;
    }

    return gateway;
}

bool gateway_connect(discord_gateway *gateway){
    if (!gateway){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_connect() - gateway is NULL\n",
            __FILE__
        );

        return false;
    }

    gateway->reconnect = false;

    if (!set_gateway_endpoint(gateway)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_connect() - set_gateway_endpoint call failed\n",
            __FILE__
        );

        return false;
    }

    char *endpoint = string_duplicate(gateway->endpoint);

    if (!endpoint){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_connect() - endpoint string alloc failed\n",
            __FILE__
        );

        return false;
    }

    const char *protocol = NULL;
    const char *address = NULL;
    int port = 0;
    const char *path = NULL;

    int err = lws_parse_uri(
        endpoint,
        &protocol,
        &address,
        &port,
        &path
    );

    if (err){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_connect() - lws_parse_uri call failed\n",
            __FILE__
        );

        free(endpoint);

        return false;
    }

    struct lws_client_connect_info conninfo = {0};
    conninfo.context = gateway->context;
    conninfo.protocol = lwsprotocols[0].name;
    conninfo.address = address;
    conninfo.port = DISCORD_GATEWAY_PORT;
    conninfo.path = path;
    conninfo.origin = address;
    conninfo.host = address;
    conninfo.ssl_connection = LCCSCF_USE_SSL;
    conninfo.pwsi = &gateway->wsi;

    log_write(
        logger,
        LOG_DEBUG,
        "[%s] gateway_connect() - connecting to %s://%s:%d/%s\n",
        __FILE__,
        protocol,
        address,
        port,
        path
    );

    struct lws *wsi = lws_client_connect_via_info(&conninfo);

    free(endpoint);

    if (!wsi){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_connect() - lws_client_connect_via_info call failed\n",
            __FILE__
        );

        return false;
    }

    return true;
}

void gateway_disconnect(discord_gateway *gateway){
    if (!gateway){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_disconnect() - gateway is NULL\n",
            __FILE__
        );

        return;
    }
    else if (!gateway->connected){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] gateway_disconnect() - gateway is not connected\n",
            __FILE__
        );

        return;
    }

    gateway->connected = false;

    cancel_gateway_heartbeating(gateway);

    lws_close_reason(
        gateway->wsi,
        gateway->resume ? 4000 : 1000,
        NULL,
        0
    );

    lws_callback_on_writable(gateway->wsi);
}

bool gateway_run_loop(discord_gateway *gateway){
    if (!gateway){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_run_loop() - gateway is NULL\n",
            __FILE__
        );

        return false;
    }

    while (gateway->running){
        int ret = lws_service(gateway->context, 0);

        if (ret){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] service_gateway() - lws_service call failed\n",
                __FILE__
            );

            return false;
        }
    }

    return true;
}

bool gateway_send(discord_gateway *gateway, discord_gateway_opcodes op, json_object *data){
    if (!gateway){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] gateway_send() - gateway is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (op != GATEWAY_OP_HEARTBEAT && is_rate_limited(gateway)){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] gateway_send() - gateway connection is rate limited (sent %d requests)\n",
            __FILE__,
            gateway->sent_count
        );

        return true;
    }
    else if (!gateway->connected){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] gateway_send() - gateway is not connected -- refusing to send payload\n",
            __FILE__
        );

        return true;
    }

    json_object *payloadobj = json_object_new_object();

    if (!payloadobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_send() - payload initialization failed\n",
            __FILE__
        );

        return false;
    }

    json_object *opobj = json_object_new_int(op);

    if (!opobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_send() - failed to create op code object\n",
            __FILE__
        );

        json_object_put(payloadobj);

        return false;
    }

    json_object_object_add(payloadobj, "op", opobj);
    json_object_object_add(payloadobj, "d", json_object_get(data));

    const char *payloadstr = json_object_to_json_string(payloadobj);

    if (!payloadstr){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_send() - json payload object to string failed\n",
            __FILE__
        );

        json_object_put(payloadobj);

        return false;
    }

    size_t payloadstrlen = strlen(payloadstr);

    size_t payloadsize = LWS_PRE + payloadstrlen + 1;
    char *payload = malloc(payloadsize);

    if (!payload){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_send() - payload alloc failed\n",
            __FILE__
        );

        json_object_put(payloadobj);

        return false;
    }

    string_copy(payloadstr, payload + LWS_PRE, payloadsize);

    list_item item = {0};
    item.type = L_TYPE_STRING;
    item.size = payloadsize;
    item.data = payload;

    bool success = list_append(gateway->queue, &item);

    json_object_put(payloadobj);

    if (!success){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] gateway_send() - payload append to queue failed\n",
            __FILE__
        );

        free(payload);

        return false;
    }

    lws_callback_on_writable(gateway->wsi);

    if (op != GATEWAY_OP_HEARTBEAT){
        gateway->last_sent = lws_now_secs();
        gateway->sent_count += 1;
    }

    return success;
}

void gateway_free(discord_gateway *gateway){
    if (!gateway){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] gateway_free() - gateway is NULL\n",
            __FILE__
        );

        return;
    }

    if (gateway->buffer){
        free(gateway->buffer->data);
        free(gateway->buffer);
    }

    list_free(gateway->queue);

    lws_context_destroy(gateway->context);

    free(gateway->endpoint);
    free(gateway);
}
