CC = gcc
DEV_CFLAGS = -fPIC -Wall -Wextra -O0 -g
LDFLAGS = -shared
OBJS = tarmac.o

.PHONY: all
all: ;

.PHONY: test
test: test.c
	$(CC) $(DEV_CFLAGS) -o test ./test.c -lpthread
	./test
	rm ./test

.PHONY: clean
clean: ;