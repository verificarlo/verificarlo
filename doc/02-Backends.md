## Backends

  * [IEEE Backend (libinterflop_ieee.so)](#ieee-backend-libinterflop_ieeeso)
  * [MCA Backends (libinterflop_mca.so and lib_interflop_mca_int.so)](#mca-backends)
  * [Bitmask Backend (libinterflop_bitmask.so)](#bitmask-backend-libinterflop_bitmaskso)
  * [Cancellation Backend (libinterflop_cancellation.so)](#cancellation-backend-libinterflop_cancellationso)
  * [VPREC Backend (libinterflop_vprec.so)](#vprec-backend-libinterflop_vprecso)


Once your program is compiled with Verificarlo, it can be instrumented with
different floating-point backends.
At least one backend must be selected when running your application,

```bash
   $ verificarlo-c *.c -o program
   $ ./program
   program: VFC_BACKENDS is empty, at least one backend should be provided
```

Backends are distributed as dynamic libraries. They are loaded with the
environment variable `VFC_BACKENDS`.

```bash
   $ VFC_BACKENDS="libinterflop_mca.so" ./program
```

Multiple backends can be loaded at the same time; they will be chained in the
order of appearance in the `VFC_BACKENDS` variable. They must be separated with
semi-colons,

```bash
   $ VFC_BACKENDS="libinterflop_ieee.so; libinterflop_mca.so" ./program"
```

Finally backends options can be configured by passing command line arguments
after each backend,

```bash
   $ VFC_BACKENDS="libinterflop_ieee.so --debug; \
                   libinterflop_mca.so --precision-binary64 10 --mode rr" \
                   ./program"
```

You could also use the environment variable `VFC_BACKENDS_FROM_FILE` to
read the value of `VFC_BACKENDS` from a file, 

```bash
   $ echo "libinterflop_ieee.so --debug; \
           libinterflop-mca.so --precision-binary64 10 --mode rr" > config.txt
   $ export VFC_BACKENDS_FROM_FILE=$PWD/config.txt
```

> :warning: `VFC_BACKENDS` takes precedence over `VFC_BACKENDS_FROM_FILE`

To suppress the messages when loading backends, export the
environment variable `VFC_BACKENDS_SILENT_LOAD`.

```bash
   $ export VFC_BACKENDS_SILENT_LOAD="True"
   $ VFC_BACKENDS="libinterflop_ieee.so; libinterflop_mca.so" ./program"
```

To turn loading backends messages back on, unset the environment variable.

```bash
   $ unset VFC_BACKENDS_SILENT_LOAD
```

To suppress the messages displayed by the logger, export the
environment variable `VFC_BACKENDS_LOGGER`.

```bash
   $ export VFC_BACKENDS_LOGGER="False"
```

To remove the color displayed by the logger, export the
environment variable `VFC_BACKENDS_COLORED_LOGGER`.

```bash
   $ export VFC_BACKENDS_COLORED_LOGGER="False"
```

To redirect the logger info, export the
environment variable `VFC_BACKENDS_LOGFILE`.
Verificarlo will suffix the name with the current TID. 

```bash
   $ export VFC_BACKENDS_LOGFILE='verificarlo.log'
   $ ./test
   $ ls
   $ verificarlo.log.3636865
```

The IEEE, MCA, Bitmask and Cancellation backends are all re-entrant.

### IEEE Backend (libinterflop_ieee.so)

The IEEE backend implements straighforward IEEE-754 arithmetic.
It should have no effect on the output and behavior of your program.

The options `--debug` and `--debug_binary` enable verbose output that print
every instrumented floating-point operation.

The option `--count-op` enable to count the dynamic number of mul/div/add/sub operations during the instrumented program execution, 
and print it on the standard error output at the end of program execution.
```bash

VFC_BACKENDS="libinterflop_ieee.so --help" ./test
Info [verificarlo]: loaded backend libinterflop_ieee.so
Usage: libinterflop_ieee.so [OPTION...]

  -b, --debug-binary         enable binary debug output
  -d, --debug                enable debug output
  -n, --print-new-line       add a new line after debug ouput
  -o, --count-op             enable operation count output
  -p, --print-subnormal-normalized
                             normalize subnormal numbers
  -s, --no-backend-name      do not print backend name in debug output
  -?, --help                 Give this help list
      --usage                Give a short usage message

VFC_BACKENDS="libinterflop_ieee.so --debug" ./test
Info [verificarlo]: loaded backend libinterflop_ieee.so
Info [interflop_ieee]: Decimal 1.23457e-05 - 9.87654e+12 -> -9.87654e+12
Info [interflop_ieee]: Decimal 1.23457e-05 * 9.87654e+12 -> 1.21933e+08
Info [interflop_ieee]: Decimal 1.23457e-05 / 9.87654e+12 -> 1.25e-18
...

VFC_BACKENDS="libinterflop_ieee.so --debug-binary --print-new-line" ./test
Info [verificarlo]: loaded backend libinterflop_ieee.so
Info [interflop_ieee]: Binary
+1.100111100100000011000001011001111111010000011 x 2^-17 -
+1.00011111011100011111010100010000111 x 2^43 ->
-1.00011111011100011111010100010000111 x 2^43

Info [interflop_ieee]: Binary
+1.100111100100000011000001011001111111010000011 x 2^-17 *
+1.00011111011100011111010100010000111 x 2^43 ->
+1.110100010010001011111111111110000011000100100110111 x 2^26

Info [interflop_ieee]: Binary
+1.100111100100000011000001011001111111010000011 x 2^-17 /
+1.00011111011100011111010100010000111 x 2^43 ->
+1.0111000011101111100001010101101010010010111010010101 x 2^-60
...

VFC_BACKENDS="libinterflop_ieee.so --count-op" ./test
Info [verificarlo]: loaded backend libinterflop_ieee.so
result is correct -9.87642e+12 == -9.87642e+12 (ref)
operations count:
         mul=2
         div=2
         add=4
         sub=2

```

### MCA Backends

The MCA backends implement Montecarlo Arithmetic.

There are two available backends:

- `libinterflop_mca.so`: uses floating point types to represent stochastic
  noise.  It uses quad type to compute MCA operations on doubles and double
  type to compute MCA operations on floats.
- `libinterflop_mca_int.so`: uses integer types to represent stochastic noise.
  In most architectures, this backend should be faster. The MCA integer backend 
  only supports default precision and relative error mode; some user options
  are therefore unavailable.

```bash
VFC_BACKENDS="libinterflop_mca.so --help" ./test
test: verificarlo loaded backend libinterflop_mca.so
Usage: libinterflop_mca.so [OPTION...]

  -m, --mode=MODE            select MCA mode among {ieee, mca, pb, rr}
      --precision-binary32=PRECISION
                             select precision for binary32 (PRECISION >= 0)
      --precision-binary64=PRECISION
                             select precision for binary64 (PRECISION >= 0)
      --error-mode=ERR_MODE  select error mode among (rel, abs, all)
      --max-abs-error-exponent=ERR_EXPONENT
                             select the magnitude of the maximum allowed
                             absolute error (this option is only used when
                             error-mode={abs, all})
  -d, --daz                  denormals-are-zero: sets denormals inputs to zero
  -f, --ftz                  flush-to-zero: sets denormal output to zero
  -s, --seed=SEED            fix the random generator seed
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Two options control the behavior of the MCA backend.

The option `--mode=MODE` controls the arithmetic error mode. It accepts the
following case insensitive values:

 * `mca`: (default mode) Montecarlo Arithmetic with inbound and outbound errors
 * `ieee`: the program uses standard IEEE arithmetic, no errors are introduced
 * `pb`: Precision Bounding inbound errors only
 * `rr`: Random Rounding outbound errors only

The option `--precision-binary64=PRECISION` controls the virtual
precision used for the floating point operations in double precision
(respectively for single precision with --precision-binary32). It
accepts an integer value that represents the virtual precision at
which MCA operations are performed. Its default value is 53 for
binary64 and 24 for binary32. A precise definition of the
virtual precision is given [here](https://hal.archives-ouvertes.fr/hal-01192668).

One should note when using the QUAD backend, that the round operations during
MCA computation always use round-to-zero mode.

In Random Round mode, the exact operations in given virtual precision are
preserved.

The option `--error-mode=ERR_MODE` controls the way in which the error is
interpreted. It accepts the following modes:

 * `rel`: (default mode) the error is specified relative to the magnitude of
 the floating-point number
 * `abs`: the error threshold is specified as an absolute value, independent of
the value of the floating-point number, to be interpreted as 2<sup>ERR_EXPONENT</sup>
 * `all`: both relative and absolute modes are active simultaneously

The option `--max-abs-error-exponent=ERR_EXPONENT` is used only when the option
`--error-mode=ERR_MODE` is active and controls the magnitude of the error
threshold, when in absolute error mode or all mode. The error thershold is set
to 2<sup>ERR_EXPONENT</sup>.

The options `--daz` and `--ftz` flush subnormal numbers to 0.
The `--daz` (**Denormals-Are-Zero**) flushes subnormal inputs to 0.
The `--ftz` (**Flush-To-Zero**) flushes subnormal output to 0.

```bash
   $ VFC_BACKENDS="libinterflop_mca.so --mode=ieee" ./test
   0x0.fffffep-126 +0x1.000000p-149 = 0x1.000000p-126
   $ VFC_BACKENDS="libinterflop_mca.so --mode=ieee --daz" ./test
   0x0.fffffep-126 +0x1.000000p-149 = 0x0
   $ VFC_BACKENDS="libinterflop_mca.so --mode=ieee --ftz" ./test
   0x0.fffffep-126 +0x1.000000p-149 = 0x1.000000p-126
```

The option `--seed` fixes the random generator seed. It should not generally be used
except if one to reproduce a particular MCA trace.


### Bitmask Backend (libinterflop_bitmask.so)

The Bitmask backend implements a fast first order model of noise. It
relies on bitmask operations to achieve low overhead. Unlike MCA backends,
the introduced noise is biased, which means that the expected value of the noise
is not equal to 0 as explained in [Chatelain's thesis, section 2.3.2](https://tel.archives-ouvertes.fr/tel-02473301/document).

```
VFC_BACKENDS="libinterflop_bitmask.so --help" ./test
test: verificarlo loaded backend libinterflop_bitmask.so
Usage: libinterflop_bitmask.so [OPTION...]

  -m, --mode=MODE            select BITMASK mode among {ieee, full, ib, ob}
  -o, --operator=OPERATOR    select BITMASK operator among {zero, one, rand}
      --precision-binary32=PRECISION
                             select precision for binary32 (PRECISION > 0)
      --precision-binary64=PRECISION
                             select precision for binary64 (PRECISION > 0)
  -d, --daz                  denormals-are-zero: sets denormals inputs to zero
  -f, --ftz                  flush-to-zero: sets denormal output to zero
  -s, --seed=SEED            fix the random generator seed
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Three options control the behavior of the Bitmask backend.

The option `--mode=MODE` controls the arithmetic error mode. It
accepts the following case insensitive values:

* `ieee`: the program uses the standard IEEE arithmetic, no errors are introduced
* `ib`: InBound precision errors only
* `ob`: OutBound precision errors only (*default mode*)
* `full`: InBound and OutBound modes combined

The option `--operator=OPERATOR` controls the bitmask operator to
apply. It accepts the following case insensitive values:

* `zero`: sets the last `t` bits of the mantissa to 0
* `one`: sets the last `t` bits of the mantissa to 1
* `rand`: applies a XOR of random bits to the last `t` bits of the mantissa (default mode)

Modes `zero` and `one` are deterministic and require only one
execution.  The `rand` mode is random and must be used like `mca`
backends.

The option `--precision-binary64=PRECISION` controls the virtual
precision used for the floating point operations in double precision
(respectively for single precision with --precision-binary32) It
accepts an integer value that represents the virtual precision at
which MCA operations are performed. Its default value is 53 for
binary64 and 24 for binary32. For the Bitmask backend, the virtual
precision corresponds to the number of preserved bits in the mantissa.

The option `--seed` fixes the random generator seed. It should not
generally be used except to reproduce a particular Bitmask
trace.

### Cancellation Backend (libinterflop_cancellation.so)

The Cancellation backend implements an automatic cancellation detector at
runtime. It is founded on difference in exponents to detect cancellation faster
than in other backend. If a cancellation is detected then the backend applies
noise on the cancelled part with the model of noise from the MCA backend. The
backend additional cost of runtime time is constant and predetermined for each
operation performed.

```
Info [verificarlo]: loaded backend libinterflop_cancellation.so
Usage: libinterflop_cancellation.so [OPTION...]

  -s, --seed=SEED            Fix the random generator seed
  -t, --tolerance=TOLERANCE  Select tolerance (TOLERANCE >= 0)
  -w, --warning=WARNING      Enable warning for cancellations
  -?, --help                 Give this help list
      --usage                Give a short usage message

```

Three options control the behavior of the Cancellation backend.

The option `--tolerance` sets the tolerance within the backend will trigger a
cancellation. By default tolerance is set to 1.

The option `--warning` warns on the standard output stream when a cancellation is
triggered by the backend.

The option `--seed` fixes the random generator seed. It should not generally be
used except if one to reproduce a particular MCA trace.

Finally the user should know that this backend is still experimental and in
developpement.

### VPREC Backend (libinterflop_vprec.so)

The VPREC backend simulates any floating-point formats that can fit into
the IEEE-754 double precision format with a round to the nearest.
The backend allows modifying the bit length of the exponent (range) and the
pseudo-mantissa (precision).

```bash
Usage: libinterflop_vprec.so [OPTION...]

  -m, --mode=MODE            select VPREC mode among {ieee, full, ib, ob}
      --precision-binary32=PRECISION
                             select precision for binary32 (PRECISION >= 0)
      --precision-binary64=PRECISION
                             select precision for binary64 (PRECISION >= 0)
      --range-binary32=RANGE select range for binary32 (0 < RANGE && RANGE <=
                             8)
      --range-binary64=RANGE select range for binary64 (0 < RANGE && RANGE <=
                             11)
      --error-mode=ERR_MODE  select error mode among (rel, abs, all)
      --max-abs-error-exponent=ERR_EXPONENT
                             select the magnitude of the maximum allowed
                             absolute error (this option is only used when
                             error-mode={abs, all})
  -d, --daz                  denormals-are-zero: sets denormals inputs to zero
  -f, --ftz                  flush-to-zero: sets denormal output to zero
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

Three options control the behavior of the VPREC backend.

The option `--mode=MODE` controls the arithmetic error mode. It accepts the following case insensitive values:

 * `ieee`: the program uses standard IEEE arithmetic, no rounding are introduced
 * `ib`: InBound precision only
 * `ob`: OutBound precision only (*default mode*)
 * `full`: Inbound and outbound mode combined

The option `--precision-binary64=PRECISION` controls the pseudo-mantissa bit length of
the new tested format for floating-point operations in double precision
(respectively for single precision with --precision-binary32).
It accepts an integer value that represents the precision at which
the rounding will be done.

The option `--range-binary64=PRECISION` controls the exponent bit length of
the new tested format for floating-point operations in double precision
(respectively for single precision with --range-binary32).
It accepts an integer value that represents the magnitude of the numbers.

The option `--error-mode=ERR_MODE` controls the way in which the error is
interpreted. It accepts the following modes:

 * `rel`: (default mode) the error is specified relative to the magnitude of
 the floating-point number
 * `abs`: the error threshold is specified as an absolute value, independent of
the value of the floating-point number, to be interpreted as 2<sup>ERR_EXPONENT</sup>
 * `all`: both relative and absolute modes are active simultaneously

The option `--max-abs-error-exponent=ERR_EXPONENT` is used only when the option
`--error-mode=ERR_MODE` is active and controls the magnitude of the error
threshold, when in absolute error mode or all mode. The error thershold is set
to 2<sup>ERR_EXPONENT</sup>.

A detailed description of the backend is given [here](https://hal.archives-ouvertes.fr/hal-02564972/document).

The following example shows the computation with single precision and the simulation of the `bfloat16` format with VPREC:

```bash
   $ VFC_BACKENDS="libinterflop_vprec.so --precision-binary32=23 --range-binary32=8" ./a.out
   (2.903225*2.903225)*16384.000000 = 138096.062500
   $ VFC_BACKENDS="libinterflop_vprec.so --precision-binary32=10 --range-binary32=5" ./a.out
   (2.903225*2.903225)*16384.000000 = inf
```


