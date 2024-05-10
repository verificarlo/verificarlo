ifndef type
$(error type is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall -DREAL=$(type) -O0 
LDFLAGS=-lm

all:
	$(CC) $(CFLAGS) -c test.c -o test_$(type).o
	$(CC) test_$(type).o -o test_$(type) $(LDFLAGS)
