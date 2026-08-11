## Interflop user call instrumentation

Verificarlo provides the ability to call low-level backend functions directly through 
the `interflop_call` function. 

### `interflop_call` signature

The `interflop_call` function has been designed to be the most generic to the user needs. 
It is declared in the `interflop.h` with the following signature:

```C
void interflop_call(interflop_call_id id, ...);
```
where `interlop_call_id` is an `enum` listing operations available.
```C
typedef enum {
  /* Allows perturbing one floating-point value */
  /* signature: void inexact(enum FTYPES type, void *value, int precision) */
  INTERFLOP_INEXACT_ID = 1,
  INTERFLOP_CUSTOM_ID = -1
} interflop_call_id;
```

The `interflop_call_id` enumeration defines the signature for each user call id.

## List of predefined user calls

### `INTERFLOP_INEXACT_ID`

Allows applying perturbation on one floating-point value.
Signature: 
```C
void interflop_call(interflop_call_id id, enum FTYPES type, void *value, int precision);
```
where:
- `id`: must be set to `INTERFLOP_INEXACT_ID`
- `type`: `enum FTYPES` that describes the type of `value`. 
- `value`: pointer to the value to perturb.
- `precision`: virtual precision to use for applying the perturbation. The behaviour depends on the sign:
  - `precision > 0 `: Use `precision` as virtual precision.
  - `precision = 0 `: Use the current virtual precision as defined by `--precision-binary{32,64}` args.
  - `precision < 0 `: Use the current virtual precision minus `precision`. (i.e. `t = MCALIB_T - precision`)

### `INTERFLOP_SET_PRECISION_BINARY64`

Allows changing the virtual precision used for floating-point operations in double precision.
For the VPREC backend, allows changing the significand bit length (including the implicit
leading bit) for floating-point operations in double precision.
Signature: 
```C
void interflop_call(interflop_call_id id, int precision);
```
where:
- `id`: must be set to `INTERFLOP_SET_PRECISION_BINARY64`
- `precision`: new virtual precision in significand bits (1–53 for VPREC binary64), must be positive.

### `INTERFLOP_SET_PRECISION_BINARY32`

Allows changing the virtual precision used for floating-point operations in single precision.
For the VPREC backend, allows changing the significand bit length (including the implicit
leading bit) for floating-point operations in single precision.
Signature: 
```C
void interflop_call(interflop_call_id id, int precision);
```
where:
- `id`: must be set to `INTERFLOP_SET_PRECISION_BINARY32`
- `precision`: new virtual precision in significand bits (1–24 for VPREC binary32), must be positive.

### `INTERFLOP_SET_RANGE_BINARY64`

Allows changing the exponent bit length for floating-point operations in double precision.
Signature: 
```C
void interflop_call(interflop_call_id id, int range);
```
where:
- `id`: must be set to `INTERFLOP_SET_RANGE_BINARY64`
- `range`: new exponent bit length (0 < range <= 11).

### `INTERFLOP_SET_RANGE_BINARY32`

Allows changing the exponent bit length for floating-point operations in single precision.
Signature: 
```C
void interflop_call(interflop_call_id id, int range);
```
where:
- `id`: must be set to `INTERFLOP_SET_RANGE_BINARY32`
- `range`: new exponent bit length (0 < range <= 8).

### `INTERFLOP_CUSTOM_ID`

General user call for custom purposes. No fixed signature.

## Constants

### `enum FTYPES`

```C
/* Enumeration of types managed by function instrumentation */
enum FTYPES {
  FFLOAT,      /* float       */
  FDOUBLE,     /* double      */
  FQUAD,       /* __float128  */
  FFLOAT_PTR,  /* float*      */
  FDOUBLE_PTR, /* double*     */
  FQUAD_PTR,   /* __float128* */
  FTYPES_END
};
```

---

## Python frontend

Verificarlo ships a Python module, `verificarlo.usercall.interflop_call`, that
exposes the same `interflop_call` interface from Python without writing any C
code. It uses `ctypes` to call the C function at runtime inside a
verificarlo-instrumented process.

### Prerequisites

The Python package must be installed (it is included in the standard verificarlo
Python package):

```bash
pip install verificarlo
```

Check that the module is importable:

```bash
python3 -c "from verificarlo.usercall.interflop_call import set_precision_binary32"
```

### Loading an instrumented library

The Python module resolves `interflop_call` lazily from the symbols that are
already loaded in the process. For this to work, the verificarlo-instrumented
shared library **must be loaded with `ctypes.RTLD_GLOBAL`** so that
`libinterflop_vfcwrapper.so` (a transitive dependency) is visible to the
resolver.

```python
import ctypes

# RTLD_GLOBAL is required for interflop_call to be visible at runtime
lib = ctypes.CDLL("./mylib.so", mode=ctypes.RTLD_GLOBAL)
```

The instrumented `.so` is built with:

```bash
verificarlo-c -shared -fPIC mylib.c -o mylib.so
```

The active backend is selected via the `VFC_BACKENDS` environment variable,
exactly as for standalone executables:

```bash
VFC_BACKENDS="libinterflop_mca.so --seed=42" python3 myscript.py
```

### High-level API

All high-level helpers are importable from `verificarlo.usercall.interflop_call`.

#### `set_precision_binary32(precision: int) -> None`

Sets the virtual significand precision for `float` (binary32) operations.

- `precision`: number of significand bits including the implicit leading bit
  (1--24 for IEEE 754 single precision).

```python
from verificarlo.usercall.interflop_call import set_precision_binary32

set_precision_binary32(10)   # reduce to 10 significand bits
result = lib.compute_float()
set_precision_binary32(24)   # restore full float precision
```

#### `set_precision_binary64(precision: int) -> None`

Sets the virtual significand precision for `double` (binary64) operations.

- `precision`: number of significand bits including the implicit leading bit
  (1--53 for IEEE 754 double precision).

```python
from verificarlo.usercall.interflop_call import set_precision_binary64

set_precision_binary64(24)   # roughly single-precision accuracy
result = lib.compute_double()
set_precision_binary64(53)   # restore full double precision
```

#### `set_range_binary32(range_val: int) -> None`

Sets the virtual exponent range for `float` (binary32) operations (VPREC
backend). Values that exceed the reduced exponent range are clipped to infinity.

- `range_val`: number of exponent bits (0 < range_val ≤ 8).

```python
from verificarlo.usercall.interflop_call import set_range_binary32

set_range_binary32(2)   # very narrow exponent range → expect overflow
```

#### `set_range_binary64(range_val: int) -> None`

Sets the virtual exponent range for `double` (binary64) operations (VPREC
backend).

- `range_val`: number of exponent bits (0 < range_val ≤ 11).

```python
from verificarlo.usercall.interflop_call import set_range_binary64

set_range_binary64(4)
```

#### `inexact_float(value: float, precision: int) -> float`

Perturbs a single `float` value through the active backend and returns the
result.

- `value`: the value to perturb.
- `precision`:
  - `> 0`: use this as the virtual precision.
  - `= 0`: use the backend's current virtual precision unchanged.
  - `< 0`: offset relative to the current virtual precision.

```python
from verificarlo.usercall.interflop_call import inexact_float

perturbed = inexact_float(3.14, 10)
```

#### `inexact_double(value: float, precision: int) -> float`

Same as `inexact_float` but for `double` values.

```python
from verificarlo.usercall.interflop_call import inexact_double

perturbed = inexact_double(3.141592653589793, 0)
```

### Context managers

#### `precision_binary32(precision: int)`

Temporarily changes the binary32 virtual precision for the duration of a
`with` block. The precision is **not** automatically restored on exit; the
caller is responsible for setting the desired precision afterwards if needed.

```python
from verificarlo.usercall.interflop_call import precision_binary32

with precision_binary32(10):
    result = lib.compute_float()
# precision remains at 10 after this block
```

#### `precision_binary64(precision: int)`

Same as `precision_binary32` but for binary64.

```python
from verificarlo.usercall.interflop_call import precision_binary64

with precision_binary64(24):
    result = lib.compute_double()
```

### Low-level API

#### `interflop_call(call_id, *args) -> None`

Direct wrapper around the C `interflop_call` function. Use this for
backend-specific (`INTERFLOP_CUSTOM_ID`) calls or when the high-level helpers
do not cover your use case.

All arguments after `call_id` **must be explicit `ctypes` objects** because the
underlying C function is variadic and Python integers/floats cannot be
marshalled automatically.

```python
import ctypes
from verificarlo.usercall.interflop_call import interflop_call, InterflopCallId

# Equivalent to set_precision_binary32(24)
interflop_call(
    InterflopCallId.INTERFLOP_SET_PRECISION_BINARY32,
    ctypes.c_int(24),
)

# Backend-specific call
interflop_call(
    InterflopCallId.INTERFLOP_CUSTOM_ID,
    ctypes.c_int(42),
    ctypes.c_double(3.14),
)
```

#### `reset() -> None`

Clears the cached `interflop_call` function pointer. Call this if you
`dlopen` a new instrumented library at runtime and need the module to
re-resolve the symbol from the updated process image.

```python
from verificarlo.usercall.interflop_call import reset

reset()
```

### Enumerations

#### `InterflopCallId`

Python mirror of the `interflop_call_id` C enum.

| Name | Value |
|---|---|
| `INTERFLOP_CUSTOM_ID` | -1 |
| `INTERFLOP_INEXACT_ID` | 1 |
| `INTERFLOP_SET_PRECISION_BINARY32` | 2 |
| `INTERFLOP_SET_PRECISION_BINARY64` | 3 |
| `INTERFLOP_SET_RANGE_BINARY32` | 4 |
| `INTERFLOP_SET_RANGE_BINARY64` | 5 |
| `INTERFLOP_SET_ROUNDING_MODE` | 6 |

#### `FType`

Python mirror of the `FTYPES` C enum, used with `interflop_call` and
`INTERFLOP_INEXACT_ID`.

| Name | Value | C type |
|---|---|---|
| `FFLOAT` | 0 | `float` |
| `FDOUBLE` | 1 | `double` |
| `FQUAD` | 2 | `__float128` |
| `FFLOAT_PTR` | 3 | `float *` |
| `FDOUBLE_PTR` | 4 | `double *` |
| `FQUAD_PTR` | 5 | `__float128 *` |

### Complete example

The following script loads a verificarlo-instrumented library, runs a
computation at full precision, reduces the precision, runs again, then perturbs
a single value.

```python
import ctypes
from verificarlo.usercall.interflop_call import (
    set_precision_binary64,
    precision_binary32,
    inexact_double,
)

# 1. Load the instrumented shared library with RTLD_GLOBAL
lib = ctypes.CDLL("./mylib.so", mode=ctypes.RTLD_GLOBAL)
lib.compute_double.restype = ctypes.c_double
lib.compute_float.restype  = ctypes.c_float

# 2. Run at full double precision (default backend setting)
full = lib.compute_double()

# 3. Run at reduced precision (≈ single-precision mantissa width)
set_precision_binary64(24)
reduced = lib.compute_double()

# 4. Scoped precision change for float operations
with precision_binary32(10):
    noisy = lib.compute_float()

# 5. Perturb a single value directly
perturbed = inexact_double(3.141592653589793, 10)

print(f"full={full}, reduced={reduced}, noisy={noisy}, perturbed={perturbed}")
```

Run it with:

```bash
VFC_BACKENDS="libinterflop_mca.so --seed=1234" python3 myscript.py
```
