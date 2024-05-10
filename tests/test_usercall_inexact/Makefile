ifndef type
$(error type is not set)
endif

ifndef N
$(error N is not set)
endif

CC=verificarlo-c
CFLAGS=-Og -Wall -DREAL=$(type) -DN=$(N)
LDFLAGS=
SOURCE=test.c
BINARY=test_$(type)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
