archimedes: archimedes.c
	verificarlo --ddebug -O0 -g archimedes.c -o archimedes -lm

dd: archimedes
	rm -rf dd.line/
	VFC_BACKENDS="libinterflop_cancellation.so --tolerance 1" vfc_ddebug ddRun ddCmp

dderrors: dd.line/rddmin-cmp/dd.line.exclude
	bash -c "vim -q <(./vfc_dderrors.py archimedes $<)"

clean:
	rm -rf archimedes dd.line *.ll *.o
