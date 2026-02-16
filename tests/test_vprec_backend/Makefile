VCC=verificarlo-c
CC=clang
CFLAGS=-g -Wall -O0
VFCFLAGS=--inst-fma
LDFLAGS=-lm

.PHONY: all check build_reference clean

all: check_output compute_vprec_rounding_float compute_vprec_rounding_double

# Run the test suite
check: all mpfr_reference
	./check.sh

# VPREC computation binaries (compiled with verificarlo-c for instrumentation)
compute_vprec_rounding_float: compute_vprec_rounding.c
	$(VCC) $(VFCFLAGS) $(CFLAGS) -DREAL=float -c $< -o compute_vprec_rounding_float.o
	$(VCC) compute_vprec_rounding_float.o -o $@ $(LDFLAGS)

compute_vprec_rounding_double: compute_vprec_rounding.c
	$(VCC) $(VFCFLAGS) $(CFLAGS) -DREAL=double -c $< -o compute_vprec_rounding_double.o
	$(VCC) compute_vprec_rounding_double.o -o $@ $(LDFLAGS)

# Checker (compiled with clang, NOT instrumented)
check_output: check_output.c fpPropImpl.c
	$(CC) $(CFLAGS) check_output.c -o check_output $(LDFLAGS)

# MPFR reference generator (compiled with clang, linked with mpfr)
compute_mpfr_rounding: compute_mpfr_rounding.c fpPropImpl.c
	$(CC) $(CFLAGS) compute_mpfr_rounding.c -o compute_mpfr_rounding $(LDFLAGS) -lmpfr

# Generate input files for all ranges
input_%.txt: generate_input.py
	python3 ./generate_input.py "input_%r.txt" 50 11

# Build MPFR reference files from scratch (requires MPFR)
build_reference: compute_mpfr_rounding input_11.txt
	rm -rf mpfr_reference
	mkdir mpfr_reference
	./compute_mpfr_rounding ./mpfr_reference
	tar cvfj mpfr_reference.tar.bz2 mpfr_reference

# Extract pre-generated MPFR reference files
mpfr_reference: mpfr_reference.tar.bz2
	tar xfj mpfr_reference.tar.bz2

clean:
	rm -rf compute_mpfr_rounding check_output
	rm -rf compute_vprec_rounding_float compute_vprec_rounding_double
	rm -rf compute_vprec_rounding_float.o compute_vprec_rounding_double.o
	rm -rf mpfr_reference input_*.txt
	rm -rf vprec_output_*.txt parallelInput log.error test.log
	rm -f .vfcwrapper*
