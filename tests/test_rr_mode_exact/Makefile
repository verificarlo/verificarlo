ifndef exp
$(error exp is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall -D$(exp) -O0
LDFLAGS=
SOURCE=rr_mode.c
BINARY=rr_mode_$(exp)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
