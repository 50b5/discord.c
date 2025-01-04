PROG = discordc
SRCS = $(wildcard *.c) $(filter-out c-utils/http.c, $(wildcard c-utils/*.c)) c-utils/hashers/spooky.c
OBJS = $(SRCS:.c=.o)

IGNORE = -Wno-implicit-fallthrough -Wno-pointer-to-int-cast \
         -Wno-format-nonliteral

DEBUGFLAGS = -Og -ggdb -DDEBUG \
             -fno-omit-frame-pointer \
             -fsanitize=address,leak,undefined

CFLAGS = -std=c18 -pedantic -Wall -Wextra -Werror $(IGNORE) $(DEBUGFLAGS)
INCLUDES = -I/usr/include -I. -I./c-utils

LDFLAGS = -L/usr/lib64 -L.
LDLIBS = -lpthread -lcurl -ljson-c -lwebsockets

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -fPIC

lib$(PROG).so: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o lib$(PROG).so $(OBJS) $(LDFLAGS) -shared $(LDLIBS) 

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(PROG) $(OBJS) $(LDFLAGS) $(LDLIBS)

.PHONY: clean
clean:
	rm -rf $(PROG) $(OBJS) *.o *.so *.core vgcore.*
