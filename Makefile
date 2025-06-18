CC = cc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
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

clean:
	rm -f build/vls build/*.o

.PHONY: all clean test
