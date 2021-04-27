CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g
LDFLAGS = -shared
OBJS = tarmac.o

.PHONY: all
all: ;

tarmac: tarmac.c tarmac.h
	$(CC) $(CFLAGS) -c tarmac.c

.PHONY: test
test: test.c
	$(CC) $(CFLAGS) -o test ./test.c -lpthread
	./test


.PHONY: clean
clean:
	rm ./tarmac.o
	rm ./test.o