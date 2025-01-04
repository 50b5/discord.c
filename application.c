#include "application.h"

static const logctx *logger = NULL;

static bool construct_application(discord_application *application){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(application->raw_object);
    struct json_object_iterator end = json_object_iter_end(application->raw_object);

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

            success = snowflake_from_string(objstr, &application->id);
        }
        else if (!strcmp(key, "name")){
            application->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "icon")){
            application->name = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "description")){
            application->description = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "rpc_origins")){
            application->rpc_origins = json_array_to_list(valueobj);

            success = application->rpc_origins;
        }
        else if (!strcmp(key, "bot_public")){
            application->bot_public = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "bot_require_code_grant")){
            application->bot_require_code_grant = json_object_get_boolean(valueobj);
        }
        else if (!strcmp(key, "terms_of_service_url")){
            application->terms_of_service_url = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "privacy_policy_url")){
            application->privacy_policy_url = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "owner")){
            application->owner = state_set_user(application->state, valueobj);

            success = application->owner;
        }
        else if (!strcmp(key, "verify_key")){
            application->verify_key = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "team")){
            application->team = team_init(application->state, valueobj);

            success = application->team;
        }
        else if (!strcmp(key, "guild_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &application->guild_id);
        }
        else if (!strcmp(key, "primary_sku_id")){
            const char *objstr = json_object_get_string(valueobj);

            success = snowflake_from_string(objstr, &application->primary_sku_id);
        }
        else if (!strcmp(key, "slug")){
            application->slug = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "cover_image")){
            application->cover_image = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "flags")){
            application->flags = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "tags")){
            application->tags = json_array_to_list(valueobj);

            success = application->tags;
        }
        else if (!strcmp(key, "install_params")){
            if (!application->install_params){
                application->install_params = malloc(sizeof(*application->install_params));

                if (!application->install_params){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_application() - install_params alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "scopes");
            application->install_params->scopes = json_array_to_list(obj);

            if (!application->install_params->scopes){
                log_write(
                    logger,
                    LOG_ERROR,
                    "[%s] construct_application() - json_array_to_list call failed for scopes\n",
                    __FILE__
                );

                success = false;

                break;
            }

            obj = json_object_object_get(valueobj, "permissions");
            application->install_params->permissions = json_object_get_string(obj);
        }
        else if (!strcmp(key, "custom_install_url")){
            application->custom_install_url = json_object_get_string(valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_application() - failed to set %s with value: %s\n",
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

discord_application *application_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] application_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] application_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_application *application = calloc(1, sizeof(*application));

    if (!application){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] application_init() - application alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    application->state = state;
    application->raw_object = json_object_get(data);

    if (!construct_application(application)){
        application_free(application);

        return NULL;
    }

    return application;
}

void application_free(void *applicationptr){
    discord_application *application = applicationptr;

    if (!application){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] application_free() - application is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(application->raw_object);

    list_free(application->rpc_origins);
    list_free(application->tags);

    team_free(application->team);

    free(application);
}
