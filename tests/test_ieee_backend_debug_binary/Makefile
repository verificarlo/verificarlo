ifndef type
$(error type is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall -DREAL=$(type) -O0 --verbose
LDFLAGS=-lm
SOURCE=test.c
BINARY=test_$(type)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
