![verificarlo logo](https://avatars1.githubusercontent.com/u/12033642)

## Verificarlo v0.2.3

[![Build Status](https://travis-ci.org/verificarlo/verificarlo.svg?branch=master)](https://travis-ci.org/verificarlo/verificarlo)
[![DOI](https://zenodo.org/badge/34260221.svg)](https://zenodo.org/badge/latestdoi/34260221)
[![Coverity](https://scan.coverity.com/projects/19956/badge.svg)](https://scan.coverity.com/projects/verificarlo-verificarlo)

A tool for automatic Montecarlo Arithmetic analysis.

## Using Verificarlo through its Docker image

A docker image is available at https://hub.docker.com/r/verificarlo/verificarlo/. 
This image uses the latest git master version of Verificarlo and includes
support for Fortran. It uses llvm-3.6.1 and gcc-4.9.

Example of usage:

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
  * LLVM, clang and opt from 3.3 up to 4.0.1, http://clang.llvm.org/
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

### Fortran support

The use of c++11 standard specific features force us to use gcc from 4.9.
Unfornately, official dragonegg repository does not provide `dragonegg.so`
for this version (4.9) and above. We plan to move to `flang` in the next release. In the meantime, if you need Fortran support you can either use the provided [docker image](https://hub.docker.com/r/verificarlo/verificarlo/) or follow the instructions below,

```bash
   $ sudo apt install gcc-4.9 gcc-4.9-plugin-dev g++-4.9 gfortran-4.9 libgfortran-4.9-dev
```

For getting llvm-3.6.1, run the following commands:

```bash
   $ wget http://releases.llvm.org/3.6.1/clang+llvm-3.6.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz
   $ tar xvf clang+llvm-3.6.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz llvm-3.6.1
   $ export LLVM_INSTALL_PATH=$PWD/llvm-3.6.1
```

For getting dragonegg.so, run the following commands:

```bash
   $ git clone -b gcc-llvm-3.6 --depth=1 https://github.com/yohanchatelain/DragonEgg.git
   $ cd DragonEgg
   $ LLVM_CONFIG=${LLVM_INSTALL_PATH}/bin/llvm-config GCC=gcc-4.9 CXX=g++-4.9 make
   $ export DRAGONEGG_PATH=$PWD/dragonegg.so
```   

Then run the configuration with the appropriate paths:

```bash
   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure --with-llvm=${LLVM_INSTALL_PATH} \
                 --with-dragonegg=${DRAGONEGG_PATH} \
                 CC=gcc-4.9 CXX=g++4.9
```

Then you can follow the normal installation process

```bash
   $ make
   $ sudo make install
```   

### Checking installation

Once installation is over, we recommend that you run the test suite to ensure
verificarlo works as expected on your system:

```bash
   $ make installcheck
```

### Example on x86_64 Ubuntu 14.04 release

If you disable dragonegg support during configure, fortran_test will be disabled and considered as passing the test.

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

### Delta debug

In order to use the delta debug features, you need to export the path
of the corresponding python packages. For example, for a global
install, this would resemble (edit for your installed Python version):

```bash
	$ export PYTHONPATH=${PYTHONPATH}:/usr/local/lib/pythonXXX.XXX/site-packages
```

You can then check if your install was successful using:

```bash
	$ make installcheck
```

Alternatively, you can make the changes required for `ddebug`
permanent by editing your `~/.bashrc`, `~/.profile` or whichever
configuration file is relevant for your system by adding the above
line, and then reloading your environment using:

```bash
	$ source ~/.bashrc
```

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

A message is displayed when a backend is loaded,

```bash
   $ VFC_BACKENDS="libinterflop_ieee.so; libinterflop_mca.so" ./program"
   $ program: verificarlo loaded backend libinterflop_ieee.so
   $ program: verificarlo loaded backend  libinterflop_mca.so
   $ program: interflop_mca: loaded backend with precision-binary32 = 24, precision-binary64 = 53 and mode = mca
```

To suppress the loading message when loading backends, export the
environment variable `VFC_BACKENDS_SILENT_LOAD`.

```bash
   $ export VFC_BACKENDS_SILENT_LOAD=""
   $ VFC_BACKENDS="libinterflop_ieee.so; libinterflop_mca.so" ./program"
```

To turn loading backends messages back on, unset the environment variable.

```bash
   $ unset VFC_BACKENDS_SILENT_LOAD
   $ VFC_BACKENDS="libinterflop_ieee.so; libinterflop_mca.so" ./program"
   $ program: verificarlo loaded backend libinterflop_ieee.so
   $ program: verificarlo loaded backend  libinterflop_mca.so
   $ program: interflop_mca: loaded backend with precision-binary32 = 24, precision-binary64 = 53 and mode = mca
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
  -n, --print-new-line       print new lines after debug output
  -s, --no-print-debug-mode  do not print debug mode before debug outputting
  -?, --help                 Give this help list
      --usage                Give a short usage message

VFC_BACKENDS="libinterflop_ieee.so --debug" ./test
test: verificarlo loaded backend libinterflop_ieee.so
interflop_ieee 1.23457e-05 - 9.87654e+12 -> -9.87654e+12
interflop_ieee 1.23457e-05 * 9.87654e+12 -> 1.21933e+08
interflop_ieee 1.23457e-05 / 9.87654e+12 -> 1.25e-18
...

VFC_BACKENDS="libinterflop_ieee.so --debug --no-print-debug-mode" ./test
test: verificarlo loaded backend libinterflop_ieee.so
1.23457e-05 - 9.87654e+12 -> -9.87654e+12
1.23457e-05 * 9.87654e+12 -> 1.21933e+08
1.23457e-05 / 9.87654e+12 -> 1.25e-18
...

VFC_BACKENDS="libinterflop_ieee.so --debug-binary --print-new-line" ./test
test: verificarlo loaded backend libinterflop_ieee.so
interflop_ieee_bin 
+1.10011110010000001001000100000111000011011111010011 x 2^-17 - 
+1.00011111011100011111101100101011011 x 2^43 -> 
-1.00011111011100011111101100101011011 x 2^43

interflop_ieee_bin 
+1.10011110010000001001000100000111000011011111010011 x 2^-17 * 
+1.00011111011100011111101100101011011 x 2^43 -> 
+1.1101000100100010110100111000011001101011001001010001 x 2^26

interflop_ieee_bin 
-1.00011111011100011111101100101011011 x 2^43 + 
+1.1101000100100010110100111000011001101011001001010001 x 2^26 -> 
-1.0001111101110001000100101001100111110110001111001101 x 2^43
...
```

### MCA Backend (libinterflop_mca.so)

The MCA backends implements Montecarlo Arithmetic.  It uses quad types to
compute MCA operations on doubles and the double type to compute MCA operations
on floats. It is much faster than the MCA-MPFR backend, but it is recent and
still experimental.

```
VFC_BACKENDS="libinterflop_mca.so --help" ./test
test: verificarlo loaded backend libinterflop_mca.so
Usage: libinterflop_mca.so [OPTION...] 

  -m, --mode=MODE            select MCA mode among {ieee, mca, pb, rr}
      --precision-binary32=PRECISION
                             select precision for binary32 (PRECISION >= 0)
      --precision-binary64=PRECISION
                             select precision for binary64 (PRECISION >= 0)
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
(respectively for single precision with --precision-binary32) It
accepts an integer value that represents the virtual precision at
which MCA operations are performed. Its default value is 53 for
binary64 and 24 for binary32. For a more precise definition of the
virtual precision, you can refer to
https://hal.archives-ouvertes.fr/hal-01192668.

One should note when using the QUAD backend, that the round operations during
MCA computation always use round-to-zero mode.

In Random Round mode, the exact operations in given virtual precision are
preserved.

The option `--seed` fixes the random generator seed. It should not generally be used
except if one to reproduce a particular MCA trace.

### MCA-MPFR Backend (libinterflop_mca_mpfr.so)

The MCA-MPFR backends is an alternative and slower implementation of Montecarlo
Arithmetic. It uses the GNU multiple precision library to compute MCA
operations. It is heavily based on mcalib MPFR backend.

MCA-MPFR backend accepts the same options than the MCA backend.

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
takes precedence over inclusion.

## Examples and Tutorial

The `tests/` directory contains various examples of Verificarlo usage.

A [tutorial](https://github.com/verificarlo/verificarlo/wiki/Tutorials) in
english and french is available.

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
Copyright (c) 2019
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
