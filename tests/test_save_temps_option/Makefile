ifdef save_tmp
FLAGS="--save-temps"
else
FLAGS=''
endif

all: one two three four five six seven eight nine ten

one:
	verificarlo-c $(FLAGS) -c -DREAL=1 test.c -o test_1.o
two:
	verificarlo-c $(FLAGS) -c -DREAL=2 test.c -o test_2.o
three:
	verificarlo-c $(FLAGS) -c -DREAL=3 test.c -o test_3.o
four:
	verificarlo-c $(FLAGS) -c -DREAL=4 test.c -o test_4.o
five:
	verificarlo-c $(FLAGS) -c -DREAL=5 test.c -o test_5.o
six:
	verificarlo-c $(FLAGS) -c -DREAL=6 test.c -o test_6.o
seven:
	verificarlo-c $(FLAGS) -c -DREAL=7 test.c -o test_7.o
eight:
	verificarlo-c $(FLAGS) -c -DREAL=8 test.c -o test_8.o
nine:
	verificarlo-c $(FLAGS) -c -DREAL=9 test.c -o test_9.o
ten:
	verificarlo-c $(FLAGS) -c -DREAL=10 test.c -o test_10.o


clean:
	rm -f *.o *.ll
