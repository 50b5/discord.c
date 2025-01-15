PROG = discordc
SRCS=$(wildcard *.c)
OBJS = $(SRCS:.c=.o)

IGNORE = -Wno-implicit-fallthrough -Wno-pointer-to-int-cast \
         -Wno-format-nonliteral

DEBUGFLAGS = -Og -ggdb -DDEBUG \
             -fno-omit-frame-pointer

CFLAGS = -std=c18 -pedantic -Wall -Wextra -Werror $(IGNORE) $(DEBUGFLAGS)
INCLUDES = -I/usr/local/include -I/usr/include -I. -I..

LDFLAGS = -L/usr/local/lib -L/usr/lib64 -L. -L../c-utils
LDLIBS = -lcutils -lpthread -lcurl -ljson-c -lwebsockets

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -fPIC

lib$(PROG).so: $(OBJS)
	$(CC) $(CFLAGS) -o lib$(PROG).so $(OBJS) $(INCLUDES) $(LDFLAGS) -shared $(LDLIBS) 

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(INCLUDES) $(LDFLAGS) $(LDLIBS)

.PHONY: clean
clean:
	rm -rf $(PROG) $(OBJS) *.o *.so *.core vgcore.*
