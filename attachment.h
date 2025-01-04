#ifndef ATTACHMENT_H
#define ATTACHMENT_H

#include "state.h"

typedef struct discord_attachment {
    discord_state *state;
    json_object *raw_object;

    snowflake id;
    const char *filename;
    const char *description;
    const char *content_type;
    int size;
    const char *url;
    const char *proxy_url;
    int height;
    int width;
    bool ephemeral;
} discord_attachment;

discord_attachment *attachment_init(discord_state *, json_object *);

void attachment_free(void *);

#endif
