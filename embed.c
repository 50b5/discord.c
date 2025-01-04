#include "embed.h"

static const logctx *logger = NULL;

static bool construct_embed_fields_from_json(discord_embed *embed, json_object *data){
    if (embed->fields){
        list_empty(embed->fields);
    }
    else {
        embed->fields = list_init();

        if (!embed->fields){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_embed_fields_from_json() - fields initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    bool success = true;

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *valueobj = json_object_array_get_idx(data, index);

        discord_embed_field *field = calloc(1, sizeof(*field));

        if (!field){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_embed_fields_from_json() - field alloc failed\n",
                __FILE__
            );

            success = false;

            break;
        }

        json_object *obj = json_object_object_get(valueobj, "name");
        field->name = json_object_get_string(valueobj);

        obj = json_object_object_get(valueobj, "value");
        field->value = json_object_get_string(valueobj);

        obj = json_object_object_get(valueobj, "inline");
        field->display_inline = json_object_get_boolean(obj);

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(field);
        item.data = field;

        success = list_append(embed->fields, &item);

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_embed_fields_from_json() - list_append call failed\n",
                __FILE__
            );

            free(field);

            break;
        }
    }

    return success;
}

static bool construct_embed_from_json(discord_embed *embed){
    bool success = true;

    struct json_object_iterator curr = json_object_iter_begin(embed->raw_object);
    struct json_object_iterator end = json_object_iter_end(embed->raw_object);

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

        if (!strcmp(key, "title")){
            embed->title = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "description")){
            embed->description = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "url")){
            embed->url = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "timestamp")){
            embed->timestamp = json_object_get_string(valueobj);
        }
        else if (!strcmp(key, "color")){
            embed->color = json_object_get_int(valueobj);
        }
        else if (!strcmp(key, "footer")){
            if (!embed->footer){
                embed->footer = calloc(1, sizeof(*embed->footer));

                if (!embed->footer){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_embed_from_json() - footer alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "text");
            embed->footer->text = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "icon_url");
            embed->footer->icon_url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "proxy_icon_url");
            embed->footer->proxy_icon_url = json_object_get_string(obj);
        }
        else if (!strcmp(key, "image")){
            if (!embed->image){
                embed->image = calloc(1, sizeof(*embed->image));

                if (!embed->image){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_embed_from_json() - image alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "url");
            embed->image->url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "proxy_url");
            embed->image->proxy_url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "height");
            embed->image->height = json_object_get_int(obj);

            obj = json_object_object_get(valueobj, "width");
            embed->image->width = json_object_get_int(obj);
        }
        else if (!strcmp(key, "thumbnail")){
            if (!embed->thumbnail){
                embed->thumbnail = calloc(1, sizeof(*embed->thumbnail));

                if (!embed->thumbnail){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_embed_from_json() - thumbnail alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "url");
            embed->thumbnail->url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "proxy_url");
            embed->thumbnail->proxy_url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "height");
            embed->thumbnail->height = json_object_get_int(obj);

            obj = json_object_object_get(valueobj, "width");
            embed->thumbnail->width = json_object_get_int(obj);
        }
        else if (!strcmp(key, "video")){
            if (!embed->video){
                embed->video = calloc(1, sizeof(*embed->video));

                if (!embed->video){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_embed_from_json() - video alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "url");
            embed->video->url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "proxy_url");
            embed->video->proxy_url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "height");
            embed->video->height = json_object_get_int(obj);

            obj = json_object_object_get(valueobj, "width");
            embed->video->width = json_object_get_int(obj);
        }
        else if (!strcmp(key, "provider")){
            if (!embed->provider){
                embed->provider = calloc(1, sizeof(*embed->provider));

                if (!embed->provider){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_embed_from_json() - provider alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "name");
            embed->provider->name = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "url");
            embed->provider->url = json_object_get_string(obj);
        }
        else if (!strcmp(key, "author")){
            if (!embed->author){
                embed->author = calloc(1, sizeof(*embed->author));

                if (!embed->author){
                    log_write(
                        logger,
                        LOG_ERROR,
                        "[%s] construct_embed_from_json() - author alloc failed\n",
                        __FILE__
                    );

                    success = false;

                    break;
                }
            }

            json_object *obj = json_object_object_get(valueobj, "name");
            embed->author->name = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "url");
            embed->author->url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "icon_url");
            embed->author->icon_url = json_object_get_string(obj);

            obj = json_object_object_get(valueobj, "proxy_icon_url");
            embed->author->proxy_icon_url = json_object_get_string(obj);
        }
        else if (!strcmp(key, "fields")){
            success = construct_embed_fields_from_json(embed, valueobj);
        }

        if (!success){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] construct_embed_from_json() - failed to set %s with value: %s\n",
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

list *embed_list_from_json_array(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_list_from_json_array() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }
    else if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_list_from_json_array() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    json_type type = json_object_get_type(data);
    size_t length = json_object_array_length(data);

    if (type != json_type_array){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_list_from_json_array() - data is type %d -- not a json array\n",
            __FILE__,
            type
        );

        return NULL;
    }
    else if (!length){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] embed_list_from_json_array() - json array is empty\n",
            __FILE__
        );

        return NULL;
    }

    list *embeds = list_init();

    if (!embeds){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_list_from_json_array() - embeds initialization failed\n",
            __FILE__
        );

        return NULL;
    }

    for (size_t index = 0; index < json_object_array_length(data); ++index){
        json_object *valueobj = json_object_array_get_idx(data, index);
        type = json_object_get_type(valueobj);

        if (type != json_type_object){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_list_from_json_array() - object (type: %d) is not a json object: %s\n",
                __FILE__,
                type,
                json_object_to_json_string(valueobj)
            );

            list_free(embeds);

            return NULL;
        }

        discord_embed *embed = embed_init(state, valueobj);

        if (!embed){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_list_from_json_array() - embed initialization failed\n",
                __FILE__
            );

            list_free(embeds);

            return NULL;
        }

        list_item item = {0};
        item.type = L_TYPE_GENERIC;
        item.size = sizeof(*embed);
        item.data = embed;
        item.generic_free = embed_free;

        if (!list_append(embeds, &item)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_list_from_json_array() - list_append call failed\n",
                __FILE__
            );

            embed_free(embed);
            list_free(embeds);

            return NULL;
        }
    }

    return embeds;
}

discord_embed *embed_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    discord_embed *embed = calloc(1, sizeof(*embed));

    if (!embed){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_init() - embed alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    embed->state = state;

    if (data){
        embed->raw_object = json_object_get(data);

        if (!construct_embed_from_json(embed)){
            embed_free(embed);

            return NULL;
        }
    }
    else {
        embed->raw_object = json_object_new_object();

        if (!embed->raw_object){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_init() - raw_object initialization failed\n",
                __FILE__
            );

            embed_free(embed);

            return NULL;
        }
    }

    return embed;
}

size_t embed_get_length(const discord_embed *embed){
    if (!embed){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_get_length() - embed is NULL\n",
            __FILE__
        );

        return 0;
    }

    size_t length = strlen(embed->title) + strlen(embed->description);

    if (embed->footer){
        length += strlen(embed->footer->text);
    }

    if (embed->author){
        length += strlen(embed->author->name);
    }

    if (embed->fields){
        for (size_t index = 0; index < list_get_length(embed->fields); ++index){
            const discord_embed_field *field = list_get_generic(embed->fields, index);

            length += strlen(field->name) + strlen(field->value);
        }
    }

    return length;
}

bool embed_set_title(discord_embed *embed, const char *title){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_title() - embed is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = NULL;

    if (title){
        obj = json_object_new_string(title);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_title() - title object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(embed->raw_object, "title", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_title() - json_object_object_add call failed for title\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->title = json_object_get_string(obj);

    return true;
}

bool embed_set_description(discord_embed *embed, const char *description){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_description() - embed is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = NULL;

    if (description){
        obj = json_object_new_string(description);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_description() - description object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(embed->raw_object, "description", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_description() - json_object_object_add call failed for description\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->description = json_object_get_string(obj);

    return true;
}

bool embed_set_url(discord_embed *embed, const char *url){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_url() - embed is NULL\n",
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
                "[%s] embed_set_url() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(embed->raw_object, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_url() - json_object_object_add call failed for url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->url = json_object_get_string(obj);

    return true;
}

bool embed_set_timestamp(discord_embed *embed, const char *timestamp){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_timestamp() - embed is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = NULL;

    if (timestamp){
        obj = json_object_new_string(timestamp);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_timestamp() - timestamp object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(embed->raw_object, "timestamp", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_timestamp() - json_object_object_add call failed for timestamp\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->timestamp = json_object_get_string(obj);

    return true;
}

bool embed_set_color(discord_embed *embed, int color){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_color() - embed is NULL\n",
            __FILE__
        );

        return false;
    }

    json_object *obj = json_object_new_int(color);

    if (!obj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_color() - color object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "color", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_color() - json_object_object_add call failed for color\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->color = json_object_get_int(obj);

    return true;
}

bool embed_set_footer(discord_embed *embed, const char *text, const char *iconurl){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_footer() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->footer){
        embed->footer = calloc(1, sizeof(*embed->footer));

        if (!embed->footer){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_footer() - footer alloc failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *footerobj = json_object_new_object();

    if (!footerobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_footer() - footer object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "footer", footerobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_footer() - json_object_object_add call failed for footer\n",
            __FILE__
        );

        json_object_put(footerobj);

        return false;
    }

    json_object *obj = NULL;

    if (text){
        obj = json_object_new_string(text);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_footer() - text object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(footerobj, "text", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_footer() - json_object_object_add call failed for text\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->footer->text = json_object_get_string(obj);
    obj = NULL;

    if (iconurl){
        obj = json_object_new_string(iconurl);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_footer() - icon_url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(footerobj, "icon_url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_footer() - json_object_object_add call failed for icon_url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->footer->icon_url = json_object_get_string(obj);

    return true;
}

bool embed_set_image(discord_embed *embed, const char *url){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_image() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->image){
        embed->image = calloc(1, sizeof(*embed->image));

        if (!embed->image){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_image() - image alloc failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *imageobj = json_object_new_object();

    if (!imageobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_image() - image object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "image", imageobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_image() - json_object_object_add call failed for image\n",
            __FILE__
        );

        json_object_put(imageobj);

        return false;
    }

    json_object *obj = NULL;

    if (url){
        obj = json_object_new_string(url);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_image() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(imageobj, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_image() - json_object_object_add call failed for url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->image->url = json_object_get_string(obj);

    return true;
}

bool embed_set_thumbnail(discord_embed *embed, const char *url){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_thumbnail() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->thumbnail){
        embed->thumbnail = calloc(1, sizeof(*embed->thumbnail));

        if (!embed->thumbnail){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_thumbnail() - thumbnail alloc failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *thumbnailobj = json_object_new_object();

    if (!thumbnailobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_thumbnail() - thumbnail object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "thumbnail", thumbnailobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_thumbnail() - json_object_object_add call failed for thumbnail\n",
            __FILE__
        );

        json_object_put(thumbnailobj);

        return false;
    }

    json_object *obj = NULL;

    if (url){
        obj = json_object_new_string(url);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_thumbnail() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(thumbnailobj, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_thumbnail() - json_object_object_add call failed for url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->thumbnail->url = json_object_get_string(obj);

    return true;
}

bool embed_set_video(discord_embed *embed, const char *url){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_video() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->video){
        embed->video = calloc(1, sizeof(*embed->video));

        if (!embed->video){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_video() - video alloc failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *videoobj = json_object_new_object();

    if (!videoobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_video() - video object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "video", videoobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_video() - json_object_object_add call failed for video\n",
            __FILE__
        );

        json_object_put(videoobj);

        return false;
    }

    json_object *obj = NULL;

    if (url){
        obj = json_object_new_string(url);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_video() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(videoobj, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_video() - json_object_object_add call failed for url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->video->url = json_object_get_string(obj);

    return true;
}

bool embed_set_provider(discord_embed *embed, const char *name, const char *url){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_provider() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->provider){
        embed->provider = calloc(1, sizeof(*embed->provider));

        if (!embed->provider){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_provider() - provider alloc failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *providerobj = json_object_new_object();

    if (!providerobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_provider() - provider object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "provider", providerobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_provider() - json_object_object_add call failed for provider\n",
            __FILE__
        );

        json_object_put(providerobj);

        return false;
    }

    json_object *obj = NULL;

    if (name){
        obj = json_object_new_string(name);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_provider() - name object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(providerobj, "name", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_provider() - json_object_object_add call failed for name\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->provider->name = json_object_get_string(obj);
    obj = NULL;

    if (url){
        obj = json_object_new_string(url);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_provider() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(providerobj, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_provider() - json_object_object_add call failed for url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->provider->url = json_object_get_string(obj);

    return true;
}

bool embed_set_author(discord_embed *embed, const char *name, const char *url, const char *iconurl){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_set_author() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->author){
        embed->author = calloc(1, sizeof(*embed->author));

        if (!embed->author){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_author() - author alloc failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *authorobj = json_object_new_object();

    if (!authorobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_author() - author object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(embed->raw_object, "author", authorobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_author() - json_object_object_add call failed for author\n",
            __FILE__
        );

        json_object_put(authorobj);

        return false;
    }

    json_object *obj = NULL;

    if (name){
        obj = json_object_new_string(name);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_author() - name object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(authorobj, "name", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_author() - json_object_object_add call failed for name\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->author->name = json_object_get_string(obj);
    obj = NULL;

    if (url){
        obj = json_object_new_string(url);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_author() - url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(authorobj, "url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_author() - json_object_object_add call failed for url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->author->url = json_object_get_string(obj);
    obj = NULL;

    if (iconurl){
        obj = json_object_new_string(iconurl);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_set_author() - icon_url object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(authorobj, "icon_url", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_set_author() - json_object_object_add call failed for icon_url\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    embed->author->icon_url = json_object_get_string(obj);

    return true;
}

bool embed_add_field(discord_embed *embed, const char *name, const char *value, bool displayinline){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_add_field() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (!embed->fields){
        embed->fields = list_init();

        if (!embed->fields){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_add_field() - fields initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    json_object *fieldsobj = json_object_object_get(embed->raw_object, "fields");

    if (!fieldsobj){
        fieldsobj = json_object_new_array();

        if (!fieldsobj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_add_field() - fields object initialization failed\n",
                __FILE__
            );

            return false;
        }

        if (json_object_object_add(embed->raw_object, "fields", fieldsobj)){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_add_field() - json_object_object_add call failed for fields\n",
                __FILE__
            );

            json_object_put(fieldsobj);

            return false;
        }
    }

    json_object *fieldobj = json_object_new_object();

    if (!fieldobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - field object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_array_add(fieldsobj, fieldobj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - json_object_array_add call failed for field\n",
            __FILE__
        );

        json_object_put(fieldobj);

        return false;
    }

    json_object *obj = NULL;

    if (name){
        obj = json_object_new_string(name);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_add_field() - name object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(fieldobj, "name", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - json_object_object_add call failed for name\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    name = json_object_get_string(obj);
    obj = NULL;

    if (value){
        obj = json_object_new_string(value);

        if (!obj){
            log_write(
                logger,
                LOG_ERROR,
                "[%s] embed_add_field() - value object initialization failed\n",
                __FILE__
            );

            return false;
        }
    }

    if (json_object_object_add(fieldobj, "value", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - json_object_object_add call failed for value\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    value = json_object_get_string(obj);

    obj = json_object_new_boolean(displayinline);

    if (!obj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - inline object initialization failed\n",
            __FILE__
        );

        return false;
    }

    if (json_object_object_add(fieldobj, "inline", obj)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - json_object_object_add call failed for inline\n",
            __FILE__
        );

        json_object_put(obj);

        return false;
    }

    discord_embed_field *field = calloc(1, sizeof(*field));

    if (!field){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - field alloc failed\n",
            __FILE__
        );

        return false;
    }

    field->name = name;
    field->value = value;
    field->display_inline = displayinline;

    list_item item = {0};
    item.type = L_TYPE_GENERIC;
    item.size = sizeof(*field);
    item.data = field;

    if (!list_append(embed->fields, &item)){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_add_field() - list_append call failed\n",
            __FILE__
        );

        free(field);

        return false;
    }

    return true;
}

bool embed_remove_field(discord_embed *embed, size_t pos){
    if (!embed){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_remove_field() - embed is NULL\n",
            __FILE__
        );

        return false;
    }
    else if (pos >= list_get_length(embed->fields)){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] embed_remove_field() - position %ld is out of bounds\n",
            __FILE__,
            pos
        );

        return false;
    }

    json_object *fieldsobj = json_object_object_get(embed->raw_object, "fields");

    if (!fieldsobj){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] embed_remove_field() - json_object_object_get returned NULL for fields\n",
            __FILE__
        );

        return false;
    }

    json_object_array_del_idx(fieldsobj, pos, 1);
    list_remove(embed->fields, pos);

    return true;
}

void embed_free(void *embedptr){
    discord_embed *embed = embedptr;

    if (!embed){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] embed_free() - embed is NULL\n",
            __FILE__
        );

        return;
    }

    json_object_put(embed->raw_object);

    free(embed->footer);
    free(embed->image);
    free(embed->thumbnail);
    free(embed->video);
    free(embed->provider);
    free(embed->author);
    list_free(embed->fields);

    free(embed);
}
