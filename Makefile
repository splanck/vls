CC = cc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
OBJS = build/main.o build/list.o
DEPS = include/list.h

all: build/vls

build/vls: $(OBJS) | build
	$(CC) $(CFLAGS) $(OBJS) -o build/vls

build/main.o: src/main.c $(DEPS) | build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/list.o: src/list.c $(DEPS) | build
	$(CC) $(CFLAGS) -c src/list.c -o build/list.o

build:
	mkdir -p build

clean:
	rm -f build/vls build/*.o

.PHONY: all clean
