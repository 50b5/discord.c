#ifndef SNOWFLAKE_H
#define SNOWFLAKE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>

typedef uint64_t snowflake;

bool snowflake_from_string(const char *, snowflake *);
char *snowflake_to_string(snowflake);

time_t snowflake_get_creation_time(snowflake);

#endif
