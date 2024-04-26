ifndef test
$(error test is not set)
endif

ifndef type
$(error type is not set)
endif

CC=verificarlo-c
CFLAGS=-g -Wall --function=applyOp_$(type)
LDFLAGS=-lm

all:
	$(CC) $(CFLAGS) -c test_$(test).c -o test_$(test)_$(type).o
	$(CC) test_$(test)_$(type).o -o test_$(test)_$(type) $(LDFLAGS)
