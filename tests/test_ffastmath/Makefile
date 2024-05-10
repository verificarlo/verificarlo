CC=verificarlo-c
CFLAGS=-Wall -O2 $(options)
LDFLAGS=

# set variable "binary" to test_1 if options is not set or empty  and to test_2 otherwise
binary = $(if $(options),test_2,test_1)

all:
	$(CC) $(CFLAGS) -c test.c -o $(binary).o
	$(CC) $(binary).o -o $(binary) $(LDFLAGS)

