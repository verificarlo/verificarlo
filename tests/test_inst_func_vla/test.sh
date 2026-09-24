#!/bin/bash

# Regression test for the --inst-func crash on functions declaring a
# variable-length array. See test.c for the mechanism.

set -eo pipefail

export VFC_BACKENDS_LOGGER=False

rm -f ./*.ll

# 1. The program must simply run, whatever the backend.
#
# The compilations are bounded: when the pass regresses it can emit invalid IR
# ("va_start called in a non-varargs function"), and LLVM's crash handler then
# hangs symbolizing its backtrace. Fail the test instead of stalling the suite.
timeout 300 verificarlo-c test.c -o test --inst-func -lm --save-temps

for backend in "libinterflop_ieee.so" \
	"libinterflop_mca.so --precision-binary64=53" \
	"libinterflop_vprec.so --mode=ob"; do

	# Run through sh -c so that bash does not print its own crash notice.
	status=0
	VFC_BACKENDS="${backend}" sh -c "./test 100" >/dev/null 2>&1 || status=$?

	if [[ ${status} -ne 0 ]]; then
		echo "VLA + --inst-func failed (exit ${status}) with ${backend}"
		exit 1
	fi
done

# 2. The stack and varargs intrinsics must be left in their calling frame:
# moving them into a hook function is what corrupted the return address.
instrumented=$(ls -t test.*.3.ll | head -1)

for intrinsic in stacksave stackrestore va_start va_end; do
	if ! grep -q "llvm\.${intrinsic}" "${instrumented}"; then
		echo "llvm.${intrinsic} is absent, the test no longer covers it"
		exit 1
	fi
	if grep -q "vfc_.*llvm\.${intrinsic}.*_hook" "${instrumented}"; then
		echo "llvm.${intrinsic} was moved into a hook function"
		exit 1
	fi
done

# 3. Conversely, floating-point intrinsics must still be instrumented; the fix
# must not block intrinsics wholesale. fma() is lowered to llvm.fma.f64 at -O2.
rm -f ./*.ll
timeout 300 verificarlo-c -O2 test.c -o test-O2 --inst-func -lm --save-temps
instrumented=$(ls -t test.*.3.ll | head -1)

if ! grep -q "vfc_.*llvm\.fma\.f64.*_hook" "${instrumented}"; then
	echo "llvm.fma.f64 is no longer instrumented"
	exit 1
fi

# Intrinsics with immediate operands cannot be moved into a generic hook:
# replacing an immediate with a hook parameter produces invalid IR.
if grep -q "llvm\.is\.fpclass" "${instrumented}" &&
	grep -q "vfc_.*llvm\.is\.fpclass.*_hook" "${instrumented}"; then
	echo "llvm.is.fpclass with an immediate operand was moved into a hook"
	exit 1
fi

# 4. Opaque pointers make every pointer the same LLVM type, so the pass has to
# recover the pointee. A string must not be handed to the backends as an array
# of floating-point values, and the VLA must still be handed over as one.
VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=args.prof" ./test 100 >/dev/null

# Argument type codes, from enum FTYPES: 3 is FFLOAT_PTR, 4 is FDOUBLE_PTR.
if ! awk -F'\t' '
	/^(input|output):/ {
		if (fn ~ /\/(printf|atoi)\// && ($3 == 3 || $3 == 4)) {
			print "string argument of " fn " profiled as a floating-point pointer"
			bad = 1
		}
		if ($3 == 4) {
			seen_double_ptr = 1
		}
		next
	}
	{ fn = $1 }
	END {
		if (!seen_double_ptr) {
			print "the variable-length array was not profiled as a binary64 pointer"
			bad = 1
		}
		exit bad + 0
	}
' args.prof; then
	cat args.prof
	exit 1
fi

# A generic pointer used as both float* and double* is ambiguous in the callee.
# The pass must fall back to the concrete type at each call site instead of
# choosing whichever body use LLVM happens to enumerate first.
VFC_BACKENDS="libinterflop_vprec.so --prec-output-file=args-o2.prof" ./test-O2 100 >/dev/null
if ! awk -F'\t' '
	/^(input|output):/ {
		if (fn ~ /\/read_mixed\// && $3 == 3) {
			seen_float_ptr = 1
		}
		if (fn ~ /\/read_mixed\// && $3 == 4) {
			seen_double_ptr = 1
		}
		next
	}
	{ fn = $1 }
	END { exit !(seen_float_ptr && seen_double_ptr) }
' args-o2.prof; then
	echo "ambiguous generic pointer was not resolved per call site"
	cat args-o2.prof
	exit 1
fi

echo "test passed"
exit 0
