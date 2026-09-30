#!/bin/bash

# Regression test for pointer argument forwarding under --inst-func.
# When a function takes a pointer argument and passes it to another function,
# getSizeOf used to enter infinite recursion.

set -eo pipefail

export VFC_BACKENDS_LOGGER=False
./clean.sh

# Bounded compilation: if the compiler hangs due to infinite recursion,
# timeout will kill it after 15 seconds.
timeout 15 verificarlo-c -O2 test.c -o test --inst-func -lm

VFC_BACKENDS="libinterflop_ieee.so" ./test > out.txt
grep -q "^2.0$" out.txt

# The size handed to the backend for a forwarded pointer must hold for every
# caller, see sizes.c. Read it from the call to vfc_enter_function in each
# hook: after the call-site id, each pointer argument is passed as
# (type, name, size, pointer).
callsite_sizes() {
	awk '
		/^@[0-9]+ = .*c".*\\00"/ {
			s = $0
			sub(/^[^"]*c"/, "", s)
			sub(/\\00".*$/, "", s)
			ids[$1] = s
			next
		}
		/call .*@vfc_enter_function\(ptr @[0-9]+,/ {
			s = $0
			sub(/^.*@vfc_enter_function\(ptr /, "", s)
			id = s
			sub(/,.*$/, "", id)
			if (!match(s, /ptr @[0-9]+, i32 [0-9]+, ptr/)) {
				next
			}
			size = substr(s, RSTART, RLENGTH)
			sub(/^.*i32 /, "", size)
			sub(/,.*$/, "", size)
			split(ids[id], f, "/")
			print f[2] "/" f[3], size
		}
	' "$1"
}

status=0
for opt in -O0 -O2; do
	rm -f sizes.*.ll
	timeout 60 verificarlo-c "${opt}" sizes.c -o sizes --inst-func --save-temps
	VFC_BACKENDS="libinterflop_ieee.so" ./sizes >/dev/null

	instrumented=$(ls -t sizes.*.2.ll | head -1)
	callsite_sizes "${instrumented}" >"sizes${opt}.txt"

	for expected in "wrapper/helper 0" "only_arr/helper 8" "taken/helper 0" \
		"rec/rec 4"; do
		if ! grep -qx "${expected}" "sizes${opt}.txt"; then
			echo "${opt}: expected call site size '${expected}', got:"
			cat "sizes${opt}.txt"
			status=1
		fi
	done
done

if [[ ${status} -ne 0 ]]; then
	exit 1
fi

echo "test passed"
exit 0
