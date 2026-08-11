# This file is part of the Verificarlo project,
# under the Apache License v2.0 with LLVM Exceptions.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# See https://llvm.org/LICENSE.txt for license information.
#
# Copyright (c) 2024
#    Verificarlo Contributors

"""Python frontend for interflop_call.

This module provides a Python interface to the interflop_call C function,
which allows user code to directly control backend behavior at runtime
(e.g., change virtual precision mid-execution).

Typical usage:
    Load a verificarlo-instrumented shared library first, then call these
    functions to adjust the backend settings:

    >>> import ctypes
    >>> lib = ctypes.CDLL("./mylib.so")  # instrumented with verificarlo
    >>> from verificarlo.usercall.interflop_call import set_precision_binary64
    >>> set_precision_binary64(24)        # reduce to ~float precision
    >>> result = lib.my_computation()
    >>> set_precision_binary64(53)        # restore full double precision

The interflop_call C signature (from interflop.h):
    void interflop_call(interflop_call_id id, ...);

Supported call IDs and their variadic arguments:
    INTERFLOP_SET_PRECISION_BINARY32(int precision)
    INTERFLOP_SET_PRECISION_BINARY64(int precision)
    INTERFLOP_SET_RANGE_BINARY32(int range)
    INTERFLOP_SET_RANGE_BINARY64(int range)
    INTERFLOP_INEXACT_ID(enum FTYPES type, void *value, int precision)
    INTERFLOP_CUSTOM_ID(...)  -- backend-specific
"""

import ctypes
import ctypes.util
import enum
from contextlib import contextmanager
from typing import Union


class InterflowCallId(enum.IntEnum):
    """Identifiers for interflop_call operations.

    Mirrors the interflop_call_id C enum from interflop.h.
    """

    INTERFLOP_CUSTOM_ID = -1
    INTERFLOP_INEXACT_ID = 1
    INTERFLOP_SET_PRECISION_BINARY32 = 2
    INTERFLOP_SET_PRECISION_BINARY64 = 3
    INTERFLOP_SET_RANGE_BINARY32 = 4
    INTERFLOP_SET_RANGE_BINARY64 = 5


class FType(enum.IntEnum):
    """Floating-point types for INTERFLOP_INEXACT_ID.

    Mirrors the FTYPES C enum from interflop.h.
    """

    FFLOAT = 0
    FDOUBLE = 1
    FQUAD = 2
    FFLOAT_PTR = 3
    FDOUBLE_PTR = 4
    FQUAD_PTR = 5


# Module-level cache for the resolved interflop_call function pointer.
_interflop_call_fn = None


def _get_interflop_call():
    """Return the interflop_call C function, loading it lazily.

    Search order:
    1. RTLD_DEFAULT (symbols already loaded in the current process), which
       covers programs that link directly against libvfcwrapper.so.
    2. An explicit dlopen of libvfcwrapper.so as a fallback.

    Raises:
        RuntimeError: If interflop_call cannot be found.
    """
    global _interflop_call_fn
    if _interflop_call_fn is not None:
        return _interflop_call_fn

    # Try to resolve the symbol from the already-loaded libraries.
    try:
        fn = ctypes.CDLL(None).interflop_call
        fn.restype = None
        _interflop_call_fn = fn
        return fn
    except AttributeError:
        pass

    # Fall back to loading libvfcwrapper explicitly.
    lib_name = (
        ctypes.util.find_library("libinterflop_vfcwrapper")
        or "libinterflop_vfcwrapper.so"
    )
    try:
        fn = ctypes.CDLL(lib_name).interflop_call
        fn.restype = None
        _interflop_call_fn = fn
        return fn
    except (OSError, AttributeError):
        pass

    raise RuntimeError(
        "interflop_call not found. "
        "Load a verificarlo-instrumented shared library before calling "
        "these functions, or make sure libinterflop_vfcwrapper.so is on LD_LIBRARY_PATH."
    )


def reset() -> None:
    """Clear the cached interflop_call function pointer.

    Call this if you reload the instrumented library at runtime.
    """
    global _interflop_call_fn
    _interflop_call_fn = None


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------


def set_precision_binary32(precision: int) -> None:
    """Set the virtual mantissa precision for binary32 (float) operations.

    Forwards to INTERFLOP_SET_PRECISION_BINARY32.

    Args:
        precision: Number of mantissa bits.  For IEEE 754 single precision
            the maximum is 23 (plus the implicit leading 1 = 24 significant
            bits total).
    """
    _get_interflop_call()(
        ctypes.c_int(InterflowCallId.INTERFLOP_SET_PRECISION_BINARY32),
        ctypes.c_int(precision),
    )


def set_precision_binary64(precision: int) -> None:
    """Set the virtual mantissa precision for binary64 (double) operations.

    Forwards to INTERFLOP_SET_PRECISION_BINARY64.

    Args:
        precision: Number of mantissa bits.  For IEEE 754 double precision
            the maximum is 52 (plus the implicit leading 1 = 53 significant
            bits total).
    """
    _get_interflop_call()(
        ctypes.c_int(InterflowCallId.INTERFLOP_SET_PRECISION_BINARY64),
        ctypes.c_int(precision),
    )


def set_range_binary32(range_val: int) -> None:
    """Set the virtual exponent range for binary32 (float) operations.

    Forwards to INTERFLOP_SET_RANGE_BINARY32.

    Args:
        range_val: Number of exponent bits.  For IEEE 754 single precision
            the maximum is 8.
    """
    _get_interflop_call()(
        ctypes.c_int(InterflowCallId.INTERFLOP_SET_RANGE_BINARY32),
        ctypes.c_int(range_val),
    )


def set_range_binary64(range_val: int) -> None:
    """Set the virtual exponent range for binary64 (double) operations.

    Forwards to INTERFLOP_SET_RANGE_BINARY64.

    Args:
        range_val: Number of exponent bits.  For IEEE 754 double precision
            the maximum is 11.
    """
    _get_interflop_call()(
        ctypes.c_int(InterflowCallId.INTERFLOP_SET_RANGE_BINARY64),
        ctypes.c_int(range_val),
    )


def inexact_float(value: float, precision: int) -> float:
    """Perturb a binary32 (float) value using the active backend.

    Forwards to INTERFLOP_INEXACT_ID with FTYPES=FFLOAT.

    The value is written into a ctypes buffer, passed by pointer to the
    backend, and the (possibly modified) value is returned.

    Args:
        value: The float to perturb.
        precision: Precision hint passed to the backend.  A value <= 0 is
            typically interpreted as an offset relative to the backend's
            current virtual precision.

    Returns:
        The perturbed float value.
    """
    c_val = ctypes.c_float(value)
    _get_interflop_call()(
        ctypes.c_int(InterflowCallId.INTERFLOP_INEXACT_ID),
        ctypes.c_int(FType.FFLOAT),
        ctypes.byref(c_val),
        ctypes.c_int(precision),
    )
    return c_val.value


def inexact_double(value: float, precision: int) -> float:
    """Perturb a binary64 (double) value using the active backend.

    Forwards to INTERFLOP_INEXACT_ID with FTYPES=FDOUBLE.

    Args:
        value: The double to perturb.
        precision: Precision hint passed to the backend.  A value <= 0 is
            typically interpreted as an offset relative to the backend's
            current virtual precision.

    Returns:
        The perturbed double value.
    """
    c_val = ctypes.c_double(value)
    _get_interflop_call()(
        ctypes.c_int(InterflowCallId.INTERFLOP_INEXACT_ID),
        ctypes.c_int(FType.FDOUBLE),
        ctypes.byref(c_val),
        ctypes.c_int(precision),
    )
    return c_val.value


def interflop_call(call_id: Union[InterflowCallId, int], *args) -> None:
    """Low-level wrapper around the C interflop_call function.

    Use this for backend-specific (INTERFLOP_CUSTOM_ID) calls or when the
    higher-level helpers do not cover your use case.

    All positional arguments after *call_id* must be explicit ctypes objects
    (e.g. ``ctypes.c_int(42)``), because the underlying C function is variadic
    and Python integers/floats cannot be automatically marshalled.

    Example::

        import ctypes
        from verificarlo.usercall.interflop_call import (
            interflop_call, InterflowCallId
        )
        interflop_call(InterflowCallId.INTERFLOP_CUSTOM_ID,
                       ctypes.c_int(42), ctypes.c_double(3.14))

    Args:
        call_id: The interflop_call_id enum value (or equivalent integer).
        *args:   ctypes-typed variadic arguments matching the backend's
                 expected signature for the given *call_id*.
    """
    _get_interflop_call()(ctypes.c_int(int(call_id)), *args)


# ---------------------------------------------------------------------------
# Context managers for scoped precision changes
# ---------------------------------------------------------------------------


@contextmanager
def precision_binary32(precision: int):
    """Context manager that temporarily changes the binary32 virtual precision.

    The previous precision is **not** automatically restored on exit because
    the wrapper does not expose a getter for the current precision.  Instead,
    the caller is responsible for passing the restore value explicitly via
    the *restore* parameter, or simply re-setting precision after the block.

    This helper is intentionally simple: it sets the precision on entry and
    yields.  No restoration is attempted — use it when you know the desired
    precision for a whole section of code.

    Example::

        with precision_binary32(10):
            result = lib.low_precision_kernel()

    Args:
        precision: Desired mantissa bit-width inside the block.
    """
    set_precision_binary32(precision)
    yield


@contextmanager
def precision_binary64(precision: int):
    """Context manager that temporarily changes the binary64 virtual precision.

    See :func:`precision_binary32` for usage notes.

    Args:
        precision: Desired mantissa bit-width inside the block.
    """
    set_precision_binary64(precision)
    yield
