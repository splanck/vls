CC = cc
CFLAGS = -Wall -Wextra -std=c99

all: build/vls

build/vls: src/main.c | build
	$(CC) $(CFLAGS) src/main.c -o build/vls

build:
	mkdir -p build

clean:
	rm -f build/vls

.PHONY: all clean
