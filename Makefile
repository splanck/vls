CC ?= cc
CFLAGS ?= -Wall -Wextra -std=c99 -Iinclude

PREFIX ?= /usr/local
DESTDIR ?=

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    PLATFORM_CFLAGS = -D_DARWIN_C_SOURCE
else ifeq ($(UNAME_S),NetBSD)
    PLATFORM_CFLAGS = -D_NETBSD_SOURCE
else ifeq ($(UNAME_S),Linux)
    PLATFORM_CFLAGS = -D_GNU_SOURCE
endif
CFLAGS += $(PLATFORM_CFLAGS)
OBJS = build/main.o build/list.o build/color.o build/args.o
DEPS = include/list.h include/color.h include/args.h

all: build/vls

build/vls: $(OBJS) | build
	$(CC) $(CFLAGS) $(OBJS) -o build/vls

build/main.o: src/main.c $(DEPS) | build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/list.o: src/list.c $(DEPS) | build
	$(CC) $(CFLAGS) -c src/list.c -o build/list.o

build/color.o: src/color.c include/color.h | build
	$(CC) $(CFLAGS) -c src/color.c -o build/color.o

build/args.o: src/args.c include/args.h | build
	$(CC) $(CFLAGS) -c src/args.c -o build/args.o

build:
	mkdir -p build

test: build/vls
	./build/vls -a > /dev/null
	./build/vls -l > /dev/null
	./build/vls --no-color > /dev/null
	@echo "Tests completed"

install: build/vls
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 build/vls $(DESTDIR)$(PREFIX)/bin/
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 644 man/vls.1 $(DESTDIR)$(PREFIX)/share/man/man1/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/vls
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/vls.1

clean:
	rm -f build/vls build/*.o

.PHONY: all clean test install uninstall
