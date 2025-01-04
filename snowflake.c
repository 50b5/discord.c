#include "snowflake.h"

#include "log.h"
#include "str.h"

#include <errno.h>
#include <stdlib.h>

#define DISCORD_EPOCH 1420070400000

bool snowflake_from_string(const char *input, snowflake *output){
    if (!input){
        DLOG(
            "[%s] snowflake_from_string() - input is NULL\n",
            __FILE__
        );

        return false;
    }

    char *end;

    errno = 0;
    uint64_t value = strtoull(input, &end, 10);

    if (errno == ERANGE || (*end != '\0' || *input == '\0')){
        DLOG(
            "[%s] snowflake_from_string() - conversion failed\n",
            __FILE__
        );

        return false;
    }

    *output = value;

    return true;
}

char *snowflake_to_string(snowflake input){
    char *snowflakestr = string_create(
        "%" PRIu64,
        input
    );

    if (!snowflakestr){
        DLOG(
            "[%s] snowflake_to_string() - snowflake string alloc failed\n",
            __FILE__
        );
    }

    return snowflakestr;
}

time_t snowflake_get_creation_time(snowflake id){
    return ((id >> 22) + DISCORD_EPOCH) / 1000;
}
