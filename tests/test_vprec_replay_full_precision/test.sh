#!/bin/bash

# Replaying a profile that was dumped at full precision must not perturb the
# program: the result has to match the IEEE baseline in every instrumentation
# mode. See test.c for the mechanism.

set -eo pipefail

export VFC_BACKENDS_LOGGER=False

verificarlo-c test.c -o test --inst-func -lm

VFC_BACKENDS="libinterflop_ieee.so" ./test 64 >baseline.txt

# Dump a profile without editing it: every argument keeps full precision.
VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=full.prof" ./test 64 >/dev/null

if [[ ! -s full.prof ]]; then
	echo "vprec produced no profile"
	exit 1
fi

# A profile is only meaningful if the pointer arguments made it in; that is
# what regressed when every pointer looked like a float pointer.
if ! grep -q "double_ptr\|	4	" full.prof; then
	echo "no binary64 pointer argument was profiled"
	cat full.prof
	exit 1
fi

# Profile precision is expressed in significand bits and must stay within the
# supported range. Reject malformed profiles before the value reaches a shift
# in the rounding primitive.
awk -F'\t' '
	BEGIN { OFS = FS }
	/^(input|output):/ && !changed {
		$4 = 0
		changed = 1
	}
	{ print }
	END { exit !changed }
' full.prof >invalid.prof

if VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=invalid.prof --instrument=arguments --mode=ib" \
	./test 64 >/dev/null 2>&1; then
	echo "invalid zero-bit profile precision was accepted"
	exit 1
fi

status=0
for mode in ib ob full; do
	for instrument in none arguments operations all; do
		VFC_BACKENDS="libinterflop_vprec.so --prec-input-file=full.prof --instrument=${instrument} --mode=${mode}" \
			./test 64 >"replay-${mode}-${instrument}.txt"

		if ! diff -q baseline.txt "replay-${mode}-${instrument}.txt" >/dev/null; then
			echo "full-precision replay differs from IEEE (mode=${mode} instrument=${instrument}):"
			diff baseline.txt "replay-${mode}-${instrument}.txt" || true
			status=1
		fi
	done
done

if [[ ${status} -ne 0 ]]; then
	exit 1
fi

echo "test passed"
exit 0
