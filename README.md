![verificarlo logo](https://avatars1.githubusercontent.com/u/12033642)

## Verificarlo v0.3.0

[![Build Status](https://travis-ci.org/verificarlo/verificarlo.svg?branch=master)](https://travis-ci.org/verificarlo/verificarlo)
[![DOI](https://zenodo.org/badge/34260221.svg)](https://zenodo.org/badge/latestdoi/34260221)
[![Coverity](https://scan.coverity.com/projects/19956/badge.svg)](https://scan.coverity.com/projects/verificarlo-verificarlo)

A tool for debugging and assessing floating point precision and reproducibility.

   * [Using Verificarlo through its Docker image](#using-verificarlo-through-its-docker-image)
   * [Installation](#installation)
      * [Example on x86_64 Ubuntu 14.04 release without Fortran support](#example-on-x86_64-ubuntu-1404-release-without-fortran-support)
      * [Fortran support](#fortran-support)
      * [Checking installation](#checking-installation)
   * [Usage](#usage)
   * [Examples and Tutorial](#examples-and-tutorial)
   * [Backends](#backends)
      * [IEEE Backend (libinterflop_ieee.so)](#ieee-backend-libinterflop_ieeeso)
      * [MCA Backend (libinterflop_mca.so)](#mca-backend-libinterflop_mcaso)
      * [MCA-MPFR Backend (libinterflop_mca_mpfr.so)](#mca-mpfr-backend-libinterflop_mca_mpfrso)
      * [Bitmask Backend (libinterflop_bitmask.so)](#bitmask-backend-libinterflop_bitmaskso)
      * [Cancellation Backend (libinterflop_cancellation.so)](#cancellation-backend-libinterflop_cancellationso)
      * [VPREC Backend (libinterflop_vprec.so)](#vprec-backend-libinterflop_vprecso)
   * [Verificarlo inclusion / exclusion options](#verificarlo-inclusion--exclusion-options)
   * [Postprocessing](#postprocessing)
   * [Pinpointing errors with delta-debug](#pinpointing-errors-with-delta-debug)
   * [Unstable branch detection](#unstable-branch-detection)
   * [Branch instrumentation](#branch-instrumentation)
   * [How to cite Verificarlo](#how-to-cite-verificarlo)
   * [Discussion Group](#discussion-group)
   * [License](#license)


## Using Verificarlo through its Docker image

A docker image is available at https://hub.docker.com/r/verificarlo/verificarlo/. 
This image uses the latest git master version of Verificarlo and includes
support for Fortran. It uses llvm-3.6.1 and gcc-4.9.

Example of usage with Monte Carlo arithmetic:

```bash
$ cat > test.c <<HERE
#include <stdio.h>
int main() {
  double a = 0;
  for (int i=0; i < 10000; i++) a += 0.1;
  printf("%0.17f\n", a);
  return 0;
}
HERE

$ docker pull verificarlo/verificarlo
$ docker run -v "$PWD":/workdir verificarlo/verificarlo \
   verificarlo test.c -o test
$ docker run -v "$PWD":/workdir -e VFC_BACKENDS="libinterflop_mca.so" \
   verificarlo/verificarlo ./test
999.99999999999795364
$ docker run -v "$PWD":/workdir -e VFC_BACKENDS="libinterflop_mca.so" \
   verificarlo/verificarlo ./test   
999.99999999999761258
```

## Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * GNU mpfr library http://www.mpfr.org/
  * LLVM, clang and opt from 3.3 up to 9.0.1, http://clang.llvm.org/
  * gcc from 4.9
  * For Fortran support see section Fortran support
  * python3 and NumPy
  * autotools (automake, autoconf)

Then run the following command inside verificarlo directory:

```bash
   $ ./autogen.sh
   $ ./configure --without-dragonegg
   $ make
   $ sudo make install
```

### Example on x86_64 Ubuntu 14.04 release without Fortran support

For example on an x86_64 Ubuntu 14.04 release, you should use the following
install procedure:

```bash
   $ sudo apt-get install libmpfr-dev clang-3.3 llvm-3.3-dev dragonegg-4.9 \
       gcc-4.9 gfortran-4.9 autoconf automake build-essential python3 python3-numpy
   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure \
       --with-dragonegg=/usr/lib/gcc/x86_64-linux-gnu/4.9/plugin/dragonegg.so \
       CC=gcc-4.9
   $ make 
   $ sudo make install
```

### Fortran support

In the upcoming release Fortran support will be provided by `flang`. In the
meantime, if you need Fortran support you can either use the provided [docker
image](https://hub.docker.com/r/verificarlo/verificarlo/) or follow the
instructions below to install `dragongegg.so` with a recent gcc.

```bash
   # Install gcc-4.9, gfortran-4.9 and llvm-3.6.1 with the following commands:
   $ sudo apt install gcc-4.9 gcc-4.9-plugin-dev g++-4.9 gfortran-4.9 libgfortran-4.9-dev
   $ wget http://releases.llvm.org/3.6.1/clang+llvm-3.6.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz
   $ tar xvf clang+llvm-3.6.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz llvm-3.6.1
   $ export LLVM_INSTALL_PATH=$PWD/llvm-3.6.1

   # Install dragonegg
   $ git clone -b gcc-llvm-3.6 --depth=1 https://github.com/yohanchatelain/DragonEgg.git
   $ cd DragonEgg
   $ LLVM_CONFIG=${LLVM_INSTALL_PATH}/bin/llvm-config GCC=gcc-4.9 CXX=g++-4.9 make
   $ export DRAGONEGG_PATH=$PWD/dragonegg.so
   
   # Install Verificarlo
   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure --with-llvm=${LLVM_INSTALL_PATH} --with-dragonegg=${DRAGONEGG_PATH} CC=gcc-4.9 CXX=g++4.9
   $ make && sudo make install
```

### Checking installation

Once installation is over, we recommend that you run the test suite to ensure
verificarlo works as expected on your system.

You may need to export the path of the installed python packages. For example,
for a global install, this would resemble (edit for your installed Python
version):

```bash
$ export PYTHONPATH=${PYTHONPATH}:/usr/local/lib/pythonXXX.XXX/site-packages
```

You can make the above change permanent by editing your `~/.bashrc`,
`~/.profile` or whichever configuration file is relevant for your system.

Then you can run the test suite with,

```bash
   $ make installcheck
```

If you disable dragonegg support during configure, Fortran tests will be
disabled and considered as passing the test.



## Usage

To automatically instrument a program with Verificarlo you must compile it using
the `verificarlo` command. First make sure that the verificarlo installation
directory is in your PATH.

Then you can use the `verificarlo` command to compile your programs. Either modify 
your makefile to use `verificarlo` as the compiler (`CC=verificarlo` and
`FC=verificarlo`) and linker (`LD=verificarlo`) or use the verificarlo command
directly:

```bash
   $ verificarlo *.c *.f90 -o ./program
```

If you are trying to compile a shared library, such as those built by the Cython
extension to Python, you can then also set the shared linker environment variable
(`LDSHARED='verificarlo -shared'`) to enable position-independent linking.

When invoked with the `--verbose` flag, verificarlo provides detailed output of
the instrumentation process.

It is important to include the necessary link flags if you use extra libraries.
For example, you should include `-lm` if you are linking against the math
library and include `-lstdc++` if you use functions in the standard C++
library.

## Examples and Tutorial

The `tests/` directory contains various examples of Verificarlo usage.

A [tutorial](https://github.com/verificarlo/verificarlo/wiki/Tutorials) is available.

## Backends

Once your program is compiled with Verificarlo, it can be instrumented with
different floating-point backends.
At least one backend must be selected when running your application, 

```bash
   $ verificarlo *.c -o program
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


### IEEE Backend (libinterflop_ieee.so)

The IEEE backend implements straighforward IEEE-754 arithmetic. 
It should have no effect on the output and behavior of your program.

The options `--debug` and `--debug_binary` enable verbose output that print
every instrumented floating-point operation.

```bash
VFC_BACKENDS="libinterflop_ieee.so --help" ./test
test: verificarlo loaded backend libinterflop_ieee.so
Usage: libinterflop_ieee.so [OPTION...]

  -b, --debug-binary         enable binary debug output
  -d, --debug                enable debug output
  -n, --print-new-line       add a new line after debug ouput
  -o, --print-subnormal-normalized
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
```

### MCA Backend (libinterflop_mca.so)

The MCA backends implements Montecarlo Arithmetic.  It uses quad type to
compute MCA operations on doubles and double type to compute MCA operations
on floats. It is much faster than the legacy MCA-MPFR backend.

```
VFC_BACKENDS="libinterflop_mca.so --help" ./test
test: verificarlo loaded backend libinterflop_mca.so
Usage: libinterflop_mca.so [OPTION...] 

  -m, --mode=MODE            select MCA mode among {ieee, mca, pb, rr}
      --precision-binary32=PRECISION
                             select precision for binary32 (PRECISION >= 0)
      --precision-binary64=PRECISION
                             select precision for binary64 (PRECISION >= 0)
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

### MCA-MPFR Backend (libinterflop_mca_mpfr.so)

The MCA-MPFR backends is an alternative and slower implementation of Montecarlo
Arithmetic. It uses the GNU multiple precision library to compute MCA
operations. It is heavily based on mcalib MPFR backend.

MCA-MPFR backend accepts the same options than the MCA backend.

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

A detailed description of the backend is given [here](https://www.researchgate.net/profile/Yohan_Chatelain/publication/335232310_Automatic_Exploration_of_Reduced_Floating-Point_Representations_in_Iterative_Methods/links/5d8e18e9a6fdcc25549f95b3/Automatic-Exploration-of-Reduced-Floating-Point-Representations-in-Iterative-Methods.pdf).

The following example shows the computation with single precision and the simulation of the `bfloat16` format with VPREC:

```bash
   $ VFC_BACKENDS="libinterflop_vprec.so --precision-binary32=23 --range-binary32=8" ./a.out
   (2.903225*2.903225)*16384.000000 = 138096.062500
   $ VFC_BACKENDS="libinterflop_vprec.so --precision-binary32=10 --range-binary32=5" ./a.out
   (2.903225*2.903225)*16384.000000 = inf
```

## Verificarlo inclusion / exclusion options

If you only wish to instrument a specific function in your program, use the
`--function` option:

```bash
   $ verificarlo *.c -o ./program --function=specificfunction
```

For more complex scenarios, a white-list / black-list mechanism is also
available through the options `--include-file INCLUSION-FILE` and
`--exclude-file EXCLUSION-FILE`.

`INCLUSION-FILE` and `EXCLUSION-FILE` are files specifying which modules and
functions should be included or excluded from Verificarlo instrumentation.
Each line has a module name followed by a function name. Both the module or
function name can be replaced by the wildcard `*`. Empty lines or lines
starting with `#` are ignored.

```
# include.txt
# this inclusion file will instrument f1 in main.c and util.c, and instrument
# f2 in util.c everything else will be excluded.
main f1
util f1
util f2

# exclude.txt
# this exclusion file will exclude f3 from all modules and all functions in
# module3.c
* f3
module3 *
```

Inclusion and exclusion files can be used together, in that case inclusion
takes precedence over exclusion.

## Postprocessing

The `postprocessing/` directory contains postprocessing tools to compute floating
point accuracy information from a set of verificarlo generated outputs.

For now we only have a VTK postprocessing tool `vfc-vtk.py` which takes multiple
VTK outputs generated with verificarlo and generates a single VTK set of files that
is enriched with accuracy information for each floating point `DataArray`.

For more information about `vfc-vtk.py`, please use the online help:

```bash
$ postprocess/vfc-vtk.py --help
```

## Pinpointing errors with Delta-Debug

[Delta-Debug (DD)](https://www.st.cs.uni-saarland.de/dd/) is a generic
bug-reduction method that allows to efficiently find a minimal set of
conditions that trigger a bug. In this case, we are going to consider the set
of floating-point instructions in the program. We are using DD to find a
minimal set of instructions responsible for the possible output instabilities
and numerical bugs.

By testing instruction sub-sets and their complement, DD is able to find
smaller failing sets step by step. DD stops when it finds a failing set where
it cannot remove any instruction. We call such a minimal set _ddmin_.  The
Delta-Debug implementation for stochastic arithmetic we use here has been
developed in the [verrou](https://github.com/edf-hpc/verrou) project.

To use delta-debug, we need to write two scripts:

   - A first script ``ddRun <output_dir>``, is responsible for running the
     program and writing its output inside the ``<output_dir>`` folder.

   - A second script ``ddCmp <reference_dir> <current_dir>``, takes as
     parameter two folders including respectively the outputs from a reference
     run and from the current run. The `ddCmp` script must return
     success when the deviation between the two runs is acceptable, and fail if
     the deviation is unacceptable.

To decide if a given set is unstable, DD will repeat the experiment by running
the program five times (the number of times can be changed by setting the
environment variable ``INTERFLOP_DD_NRUNS``).

``ddRun`` and ``ddCmp`` depend on the user's application and the error
tolerance of the application domain; therefore it is hard to provide a generic
script that fits all cases. That is why we require the user to manually write
these scripts.  Once the scripts are written, the Delta-Debug session is
launched using the following command:

```bash
$ VFC_BACKENDS="libinterflop_mca.so -p 53 -m mca" vfc_ddebug ddRun ddCmp
```

The `VFC_BACKENDS` variable selects the noise model among the available
backends. Delta-debug can be used with any backend that simulates numerical
noise (`mca`, `mca_mpfr`, `cancellation`, `bitmask`, ...) .  Here as an
example, we use the MCA backend with a precision of 53 in full mca mode.
`vfc_ddebug` is the delta-debug orchestration script.

`vfc_ddebug` will test instruction sub-sets. Each time an irreductible `ddmin`
set is found it is signaled to the user and asigned a number `ddmin0`,
`ddmin1`, ...., `ddminX`. The faulty instructions of `ddminX` set are stored in
the `dd.line/ddminX/dd.line.include` (these are the instructions that were
instrumented with the noise backend during the run).

The union of the _"culprit"_ instructions can also be found in
`dd.line/rddmin-cmp/dd.line.exclude`.

A full example demonstrating delta-debug usage can be found in the
[tutorial](https://github.com/verificarlo/verificarlo/wiki/Tutorials) and in
the `tests/test_ddebug_archimedes`.

As an example, in the `test_ddebug_archimedes`, two ddmin sets are found:

```
$ cat dd.line/ddmin0/dd.line.include
0x000000000040136c: archimedes at archimedes.c:16
$ cat dd.line/ddmin1/dd.line.include
0x0000000000401399: archimedes at archimedes.c:17
```

indicating that the two instructions at lines `archimedes.c:16` and
`archimedes.c:17` are responsible for the numerical instability. The first
number indicates the exact assembly instruction address.

It is possible to highlight faulty instructions inside your code editor by
using a script such as `tests/test_ddebug_archimedes/vfc_dderrors.py`, which
returns a [quickfix](http://vimdoc.sourceforge.net/htmldoc/quickfix.html)
compatible output with the union of _ddmin_ instructions.


## Unstable branch detection

It is possible to use Verificarlo to detect branches that are unstable due to
numerical errors.  To detect unstable branches we rely on
[llvm-cov](https://llvm.org/docs/CommandGuide/llvm-cov.html) coverage reports.
To activate coverage mode in verificarlo, you should use the `--coverage` flag.

This is demonstrated in
[`tests/test_unstable_branches/`](https://github.com/verificarlo/verificarlo/tree/master/tests/test_unstable_branches/);
the idea first introduced by [verrou](https://github.com/edf-hpc/verrou), is to
compare coverage reports between multiple IEEE executions and multiple MCA
executions.

Branches that are unstable only under MCA noise, are identified as numerically
unstable.

## Branch instrumentation

Verificarlo can instrument floating point comparison operations. By default,
comparison operations are not instrumented and default backends do not make use of
this feature. If your backend requires instrumenting floating point comparisons, you
must call `verificarlo` with the `--inst-fcmp` flag.

## How to cite Verificarlo


If you use Verificarlo in your research, please cite the following paper:

    @inproceedings{verificarlo,
    author    = {Christophe Denis and
                 Pablo de Oliveira Castro and
                 Eric Petit},
    title     = {Verificarlo: Checking Floating Point Accuracy through Monte Carlo
                 Arithmetic},
    booktitle = {23nd {IEEE} Symposium on Computer Arithmetic, {ARITH} 2016, Silicon
                 Valley, CA, USA, July 10-13, 2016},
    pages     = {55--62},
    year      = {2016},
    url       = {http://dx.doi.org/10.1109/ARITH.2016.31},
    doi       = {10.1109/ARITH.2016.31},
    }

A preprint is available at https://hal.archives-ouvertes.fr/hal-01192668/file/verificarlo-preprint.pdf.

Thanks !

## Discussion Group

For questions, feedbacks or discussions about Verificarlo you can join our group at,

https://groups.google.com/forum/#!forum/verificarlo

## License
Copyright (c) 2019-2020
   Verificarlo Contributors

Copyright (c) 2018
   Universite de Versailles St-Quentin-en-Yvelines

Copyright (c) 2015
   Universite de Versailles St-Quentin-en-Yvelines
   CMLA, Ecole Normale Superieure de Cachan

Verificarlo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Verificarlo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.
