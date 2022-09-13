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
void interflop_call_id(interflop_call_id id, enum FTYPES type, void *value, int precision);
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
For the VPREC backend, allows changing the lenght of the pseudo-mantissa for floating-point operations in double precision.
Signature: 
```C
void interflop_call_id(interflop_call_id id, int precision);
```
where:
- `id`: must be set to `INTERFLOP_SET_PRECISION_BINARY64`
- `precision`: new virtual precision (pseudo-mantissa bit lenght in VPREC), must be positive.

### `INTERFLOP_SET_PRECISION_BINARY32`

Allows changing the virtual precision used for floating-point operations in single precision.
For the VPREC backend, allows changing the lenght of the pseudo-mantissa for floating-point operations in single precision.
Signature: 
```C
void interflop_call_id(interflop_call_id id, int precision);
```
where:
- `id`: must be set to `INTERFLOP_SET_PRECISION_BINARY32`
- `precision`: new virtual precision (pseudo-mantissa bit lenght in VPREC), must be positive.

### `INTERFLOP_SET_RANGE_BINARY64`

Allows changing the exponent bit lenght for floating-point operations in double precision.
Signature: 
```C
void interflop_call_id(interflop_call_id id, int range);
```
where:
- `id`: must be set to `INTERFLOP_SET_RANGE_BINARY64`
- `range`: new exponent bit lenght (0 < range <= 11).

### `INTERFLOP_SET_RANGE_BINARY32`

Allows changing the exponent bit lenght for floating-point operations in single precision.
Signature: 
```C
void interflop_call_id(interflop_call_id id, int range);
```
where:
- `id`: must be set to `INTERFLOP_SET_RANGE_BINARY32`
- `range`: new exponent bit lenght (0 < range <= 8).

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