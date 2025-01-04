#include "reaction.h"

static const logctx *logger = NULL;

discord_reaction *reaction_init(discord_state *state, json_object *data){
    if (!state){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] reaction_init() - state is NULL\n",
            __FILE__
        );

        return NULL;
    }

    logger = state->log;

    if (!data){
        log_write(
            logger,
            LOG_WARNING,
            "[%s] reaction_init() - data is NULL\n",
            __FILE__
        );

        return NULL;
    }

    discord_reaction *reaction = calloc(1, sizeof(*reaction));

    if (!reaction){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] reaction_init() - reaction alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    json_object *obj = json_object_object_get(data, "count");
    reaction->count = json_object_get_int(obj);

    obj = json_object_object_get(data, "me");
    reaction->me = json_object_get_boolean(obj);

    obj = json_object_object_get(data, "emoji");
    reaction->emoji = state_set_emoji(state, obj);

    if (!reaction->emoji){
        log_write(
            logger,
            LOG_ERROR,
            "[%s] reaction_init() - state_set_emoji call failed for %s\n",
            __FILE__,
            json_object_to_json_string(obj)
        );

        reaction_free(reaction);

        return NULL;
    }

    return reaction;
}

void reaction_free(void *ptr){
    discord_reaction *reaction = ptr;

    if (!reaction){
        log_write(
            logger,
            LOG_DEBUG,
            "[%s] reaction_free() - reaction is NULL\n",
            __FILE__
        );

        return;
    }

    free(reaction);
}
