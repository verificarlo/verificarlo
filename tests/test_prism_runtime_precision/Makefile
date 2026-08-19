PRISM_BACKEND ?= sr

CC = verificarlo-c

CFLAGS  = -O2 -fopenmp=libomp --prism-backend=$(PRISM_BACKEND)
LDFLAGS = -fopenmp=libomp --prism-backend=$(PRISM_BACKEND)

test: test.c
	$(CC) $(CFLAGS) -c test.c -o test.o
	$(CC) test.o -o test $(LDFLAGS)

clean:
	rm -f test test.o *.log
