discord.c
=========
Discord API library written in C that depends on libcurl, libwebsockets, and json-c.

Supports:
    - HTTP API requests to *all* endpoints
    - gateway connection with event callbacks (using the default libwebsockets event loop)
    - rate limit handling for both the HTTP API and the gateway connection
    - reconnect logic (read notes)
    - cache of gateway and HTTP API data

Planned to support:
    - voice support (after most of the API is covered)
    - eventually all API objects
 
Not currently planned to support:
    - interactions/slash commands as I can't use them
    - sharding since I don't have a bot in a large amount of servers and can't reliably test new code

NOTES
-----
- Reconnect logic is stable but will try to infinitely reconnect unless an error is hit. This is low priority since the idea is to keep the bot running but it will be "fixed" eventually
- The HTTP API can be used without ever connecting to the gateway. This is because I sometimes need to send messages from the terminal without eating memory with a gateway connection.

Example
=======
.. code :: c

    #include "discord.h"
    #include <stdio.h>

    static bool on_ready(void *clientptr, const void *data){
        discord *client = clientptr;
        const discord_user *user = data;

        printf("%s#%s has logged in\n", user->username, user->discriminator);

        return true;
    }

    static bool on_message_create(void *clientptr, const void *data){
        discord *client = clientptr;
        const discord_message *message = data;

        if (message->author->id == client->user->id){
            return true;
        }

        printf(
            "%s#%s: %s\n",
            message->author->username,
            message->author->discriminator,
            message->content
        );

        return true;
    }

    static const discord_gateway_events callbacks[] = {
        {"READY", on_ready},

        {"MESSAGE_CREATE", on_message_create},

        {NULL, NULL}
    }

    int main(void){
        discord_gateway_presence presence = {0};
        presence.status = "dnd";

        discord_options opts = {0};
        opts.presence = &presence;
        opts.intent = INTENT_ALL;
        opts.events = callbacks;

        discord *bot = discord_init("token", &opts);

        discord_connect_gateway(bot); /* blocks */

        discord_free(bot);
    }
