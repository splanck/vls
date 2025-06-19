CC ?= cc
CFLAGS ?= -Wall -Wextra -std=c99 -Iinclude
LDFLAGS ?=

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
SELINUX_TEST := $(shell mkdir -p build; echo 'int main(void){return 0;}' > build/selinux.c; if $(CC) $(CFLAGS) build/selinux.c -o build/selinux_test -lselinux >/dev/null 2>&1; then echo 1; else echo 0; fi; rm -f build/selinux.c build/selinux_test)
ifeq ($(SELINUX_TEST),1)
    CFLAGS += -DHAVE_SELINUX=1
    LDFLAGS += -lselinux
else
    CFLAGS += -DHAVE_SELINUX=0
endif
OBJS = build/main.o build/list.o build/color.o build/args.o build/util.o build/quote.o
DEPS = include/list.h include/color.h include/args.h include/util.h include/quote.h

all: build/vls

build/vls: $(OBJS) | build
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o build/vls

build/main.o: src/main.c $(DEPS) | build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/list.o: src/list.c $(DEPS) | build
	$(CC) $(CFLAGS) -c src/list.c -o build/list.o

build/color.o: src/color.c include/color.h | build
	$(CC) $(CFLAGS) -c src/color.c -o build/color.o

build/args.o: src/args.c include/args.h | build
	$(CC) $(CFLAGS) -c src/args.c -o build/args.o

build/util.o: src/util.c include/util.h | build
	$(CC) $(CFLAGS) -c src/util.c -o build/util.o

build/quote.o: src/quote.c include/quote.h | build
	$(CC) $(CFLAGS) -c src/quote.c -o build/quote.o

build:
	mkdir -p build

test: build/vls
	./build/vls -a > /dev/null
	./build/vls -l > /dev/null
	./build/vls --color=never > /dev/null
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
