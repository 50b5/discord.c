#ifndef EMBED_H
#define EMBED_H

#include "state.h"

typedef struct discord_embed_footer {
    const char *text;
    const char *icon_url;
    const char *proxy_icon_url;
} discord_embed_footer;

typedef struct discord_embed_image {
    const char *url;
    const char *proxy_url;
    int height;
    int width;
} discord_embed_image;

typedef struct discord_embed_thumbnail {
    const char *url;
    const char *proxy_url;
    int height;
    int width;
} discord_embed_thumbnail;

typedef struct discord_embed_video {
    const char *url;
    const char *proxy_url;
    int height;
    int width;
} discord_embed_video;

typedef struct discord_embed_provider {
    const char *name;
    const char *url;
} discord_embed_provider;

typedef struct discord_embed_author {
    const char *name;
    const char *url;
    const char *icon_url;
    const char *proxy_icon_url;
} discord_embed_author;

typedef struct discord_embed_field {
    const char *name;
    const char *value;
    bool display_inline;
} discord_embed_field;

typedef struct discord_embed {
    discord_state *state;
    json_object *raw_object;

    const char *title;
    const char *description;
    const char *url;
    const char *timestamp;
    int color;
    discord_embed_footer *footer;
    discord_embed_image *image;
    discord_embed_thumbnail *thumbnail;
    discord_embed_video *video;
    discord_embed_provider *provider;
    discord_embed_author *author;
    list *fields;
} discord_embed;

list *embed_list_from_json_array(discord_state *, json_object *);
discord_embed *embed_init(discord_state *, json_object *);

size_t embed_get_length(const discord_embed *);

bool embed_set_title(discord_embed *, const char *);
bool embed_set_description(discord_embed *, const char *);
bool embed_set_url(discord_embed *, const char *);
bool embed_set_timestamp(discord_embed *, const char *);
bool embed_set_color(discord_embed *, int);

bool embed_set_footer(discord_embed *, const char *, const char *);
bool embed_set_image(discord_embed *, const char *);
bool embed_set_thumbnail(discord_embed *, const char *);
bool embed_set_video(discord_embed *, const char *);
bool embed_set_provider(discord_embed *, const char *, const char *);
bool embed_set_author(discord_embed *, const char *, const char *, const char *);

bool embed_add_field(discord_embed *, const char *, const char *, bool);
bool embed_remove_field(discord_embed *, size_t);

void embed_free(void *);

#endif
