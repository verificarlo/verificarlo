ifndef type
$(error type is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall -DREAL=$(type) -O0 
LDFLAGS=
SOURCE=test_options.c
BINARY=test_options_$(type)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
