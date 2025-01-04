#ifndef REACTION_H
#define REACTION_H

#include "state.h"

typedef struct discord_reaction {
    int count;
    bool me;
    const discord_emoji *emoji;
} discord_reaction;

discord_reaction *reaction_init(discord_state *, json_object *);

void reaction_free(void *);

#endif
