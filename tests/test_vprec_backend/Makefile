ifndef type
$(error type is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall --verbose -DREAL=$(type) -O0
LDFLAGS=
SOURCE=compute_vprec_rounding.c
BINARY=compute_vprec_rounding_$(type)

all:
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(BINARY).o
	$(CC) $(BINARY).o -o $(BINARY) $(LDFLAGS)
