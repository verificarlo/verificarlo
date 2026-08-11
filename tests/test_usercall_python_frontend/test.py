# Tests for the Python frontend of interflop_call.
#
# Usage:
#   python3 test.py unit
#   VFC_BACKENDS="libinterflop_mca.so --seed=1234" python3 test.py mca ./libcompute.so
#   VFC_BACKENDS="libinterflop_vprec.so ..." python3 test.py vprec ./libcompute.so

import ctypes
import math
import sys

from verificarlo.usercall.interflop_call import (
    FType,
    InterflowCallId,
    inexact_double,
    inexact_float,
    interflop_call,
    precision_binary32,
    precision_binary64,
    reset,
    set_precision_binary32,
    set_precision_binary64,
    set_range_binary32,
    set_range_binary64,
)

NSAMPLES = 50  # number of samples used to detect stochastic noise


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


# ---------------------------------------------------------------------------
# Unit tests -- no backend required
# ---------------------------------------------------------------------------


def test_unit():
    """Verify enum values and module-level helpers without a live backend."""

    # InterflowCallId values must match the C enum in interflop.h
    assert InterflowCallId.INTERFLOP_CUSTOM_ID == -1, "CUSTOM_ID should be -1"
    assert InterflowCallId.INTERFLOP_INEXACT_ID == 1, "INEXACT_ID should be 1"
    assert InterflowCallId.INTERFLOP_SET_PRECISION_BINARY32 == 2
    assert InterflowCallId.INTERFLOP_SET_PRECISION_BINARY64 == 3
    assert InterflowCallId.INTERFLOP_SET_RANGE_BINARY32 == 4
    assert InterflowCallId.INTERFLOP_SET_RANGE_BINARY64 == 5

    # FType values must match the C enum in interflop.h
    assert FType.FFLOAT == 0
    assert FType.FDOUBLE == 1
    assert FType.FQUAD == 2
    assert FType.FFLOAT_PTR == 3
    assert FType.FDOUBLE_PTR == 4
    assert FType.FQUAD_PTR == 5

    # reset() should clear the module-level cache without raising
    reset()

    print("unit tests passed")


# ---------------------------------------------------------------------------
# Integration tests with MCA backend
# ---------------------------------------------------------------------------


def _load_lib(lib_path):
    """Load the verificarlo-instrumented shared library.

    RTLD_GLOBAL is required so that the interflop_call symbol provided by
    libinterflop_vfcwrapper (a transitive dependency of the .so) is visible
    in RTLD_DEFAULT, which is where _get_interflop_call() looks first.
    """
    lib = ctypes.CDLL(lib_path, mode=ctypes.RTLD_GLOBAL)
    lib.compute_float.restype = ctypes.c_float
    lib.compute_double.restype = ctypes.c_double
    return lib


def test_mca(lib_path):
    """Tests that require a live MCA backend (precision and inexact APIs)."""
    lib = _load_lib(lib_path)

    # ------------------------------------------------------------------
    # inexact_float / inexact_double
    # ------------------------------------------------------------------

    # At virtual precision 0 (= current backend precision = 24 bits for float),
    # MCA in RR mode preserves every IEEE 754 single-precision value exactly.
    x_f = ctypes.c_float(0.1).value  # exact float nearest to 0.1
    if inexact_float(x_f, 0) != x_f:
        fail("inexact_float(x, 0) should not perturb x at full float precision")

    # At virtual precision 1 (minimum), strong noise must appear in at least
    # one of NSAMPLES draws.
    noisy_f = [inexact_float(x_f, 1) for _ in range(NSAMPLES)]
    if not any(r != x_f for r in noisy_f):
        fail(
            f"inexact_float(x, 1) never perturbed x over {NSAMPLES} samples -- "
            "noise expected at precision 1"
        )

    # Same for double.
    x_d = 0.1
    if inexact_double(x_d, 0) != x_d:
        fail("inexact_double(x, 0) should not perturb x at full double precision")

    noisy_d = [inexact_double(x_d, 1) for _ in range(NSAMPLES)]
    if not any(r != x_d for r in noisy_d):
        fail(
            f"inexact_double(x, 1) never perturbed x over {NSAMPLES} samples -- "
            "noise expected at precision 1"
        )

    # ------------------------------------------------------------------
    # set_precision_binary32
    # ------------------------------------------------------------------
    ref_f = lib.compute_float()
    set_precision_binary32(10)
    low_prec_f = lib.compute_float()
    if ref_f == low_prec_f:
        fail(
            "set_precision_binary32(10) did not change compute_float() result; "
            f"both returned {ref_f}"
        )

    # ------------------------------------------------------------------
    # set_precision_binary64
    # ------------------------------------------------------------------
    ref_d = lib.compute_double()
    set_precision_binary64(10)
    low_prec_d = lib.compute_double()
    if ref_d == low_prec_d:
        fail(
            "set_precision_binary64(10) did not change compute_double() result; "
            f"both returned {ref_d}"
        )

    # ------------------------------------------------------------------
    # precision_binary32 context manager
    # ------------------------------------------------------------------
    with precision_binary32(5):
        ctx_f = lib.compute_float()
    if ctx_f == ref_f:
        fail(
            "precision_binary32(5) context manager did not affect compute_float(); "
            f"both returned {ref_f}"
        )

    # ------------------------------------------------------------------
    # precision_binary64 context manager
    # ------------------------------------------------------------------
    with precision_binary64(5):
        ctx_d = lib.compute_double()
    if ctx_d == ref_d:
        fail(
            "precision_binary64(5) context manager did not affect compute_double(); "
            f"both returned {ref_d}"
        )

    # ------------------------------------------------------------------
    # Low-level interflop_call wrapper
    # ------------------------------------------------------------------
    # Calling with INTERFLOP_SET_PRECISION_BINARY32 via the low-level API
    # should produce the same effect as set_precision_binary32.
    interflop_call(InterflowCallId.INTERFLOP_SET_PRECISION_BINARY32, ctypes.c_int(23))
    result_via_lowlevel = lib.compute_float()
    interflop_call(InterflowCallId.INTERFLOP_SET_PRECISION_BINARY32, ctypes.c_int(10))
    result_via_helper = lib.compute_float()
    if result_via_lowlevel == result_via_helper:
        fail(
            "Low-level interflop_call with different precisions produced identical "
            f"results: {result_via_lowlevel}"
        )

    # ------------------------------------------------------------------
    # reset() -- clearing the cache does not break subsequent calls
    # ------------------------------------------------------------------
    reset()
    # After reset, a new call should re-resolve interflop_call from the
    # already-loaded process symbols and succeed without error.
    set_precision_binary32(23)

    print("mca tests passed")


# ---------------------------------------------------------------------------
# Integration tests with vprec backend
# ---------------------------------------------------------------------------


def test_vprec(lib_path):
    """Tests that require a live vprec backend (range APIs)."""
    lib = _load_lib(lib_path)

    # ------------------------------------------------------------------
    # set_range_binary32 -- reducing the exponent range clips the result to inf
    # ------------------------------------------------------------------
    ref_f = lib.compute_float()
    set_range_binary32(2)
    clipped_f = lib.compute_float()
    if ref_f == clipped_f:
        fail(
            "set_range_binary32(2) did not change compute_float() result; "
            f"both returned {ref_f}"
        )
    if not (math.isinf(clipped_f) or abs(clipped_f) < abs(ref_f)):
        fail(
            f"Expected overflow or value reduction after set_range_binary32(2); "
            f"got {clipped_f}"
        )

    # ------------------------------------------------------------------
    # set_range_binary64
    # ------------------------------------------------------------------
    ref_d = lib.compute_double()
    set_range_binary64(2)
    clipped_d = lib.compute_double()
    if ref_d == clipped_d:
        fail(
            "set_range_binary64(2) did not change compute_double() result; "
            f"both returned {ref_d}"
        )
    if not (math.isinf(clipped_d) or abs(clipped_d) < abs(ref_d)):
        fail(
            f"Expected overflow or value reduction after set_range_binary64(2); "
            f"got {clipped_d}"
        )

    print("vprec tests passed")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} unit|mca|vprec [libcompute.so]")
        sys.exit(1)

    mode = sys.argv[1]
    if mode == "unit":
        test_unit()
    elif mode == "mca":
        if len(sys.argv) < 3:
            print(f"Usage: {sys.argv[0]} mca <libcompute.so>")
            sys.exit(1)
        test_mca(sys.argv[2])
    elif mode == "vprec":
        if len(sys.argv) < 3:
            print(f"Usage: {sys.argv[0]} vprec <libcompute.so>")
            sys.exit(1)
        test_vprec(sys.argv[2])
    else:
        print(f"Unknown mode: {mode}")
        sys.exit(1)
