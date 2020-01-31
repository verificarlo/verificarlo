all: cvPlot.pdf dd

unitTest: unitTest.cxx integrate.hxx
	verificarlo --ddebug -Wall -g -O3 -o $@ $< -lstdc++ -lm

cvPlot.pdf: unitTest cvPlot.gp
	./cvPlot

dd: unitTest ddRun ddCmp
	rm -rf dd.*
	VFC_BACKENDS="libinterflop_mca.so --mode rr --precision-binary32 24" vfc_ddebug ddRun ddCmp

dd2: unitTest ddRun2 ddCmp2
	rm -rf dd.*
	INTERFLOP_DD_NRUNS=1 VFC_BACKENDS="libinterflop_mca.so --mode rr --precision-binary32 24" vfc_ddebug ddRun2 ddCmp2

clean:
	rm -rf unitTest *.o *.dat dd.line
