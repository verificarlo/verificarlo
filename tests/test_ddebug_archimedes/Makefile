# Default noise MCA mode: mca at 53 precision
MCA_MODE=-m mca --precision-binary64=53

archimedes: archimedes.c
	verificarlo-c --ddebug -O0 -g archimedes.c -o archimedes -lm

dd: archimedes
	rm -rf dd.line/
	VFC_BACKENDS="libinterflop_mca.so ${MCA_MODE}" vfc_ddebug ddRun ddCmp

dderrors: dd.line/rddmin-cmp/dd.line.exclude
	bash -c "vim -q <(./vfc_dderrors.py archimedes $<)"

clean:
	rm -rf archimedes dd.line *.ll *.o
