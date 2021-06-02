all:clang gcc mca

clang:
	${LLVM_BINDIR}/clang -Wall -Wextra  -O3 -march=native print.c operation.c test.c -g -o test-clang -I.

gcc:
	${GCC_PATH} -Wall -Wextra  -O3 -march=native print.c operation.c test.c -g -o test-gcc -I.

mca:
	verificarlo-c -Wall -Wextra -O3 -march=native print.c operation.c test.c  --verbose -o test-mca --save-temps --show-cmd

clean:
	rm -f *.o *.ll *.log test-*
