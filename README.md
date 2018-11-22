![verificarlo logo](https://avatars1.githubusercontent.com/u/12033642)

## Verificarlo v0.2.1

[![Build Status](https://travis-ci.org/verificarlo/verificarlo.svg?branch=master)](https://travis-ci.org/verificarlo/verificarlo)

A tool for automatic Montecarlo Arithmetic analysis.

### Using Verificarlo through its Docker image

A docker image is available at https://hub.docker.com/r/verificarlo/verificarlo/. 
This image uses the last git master version of Verificarlo and includes support for Fortran and uses llvm-3.5 and gcc-4.7.

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
$ docker run -v $PWD:/workdir verificarlo/verificarlo \
   verificarlo test.c -o test
$ docker run -v $PWD:/workdir verificarlo/verificarlo \
   ./test
999.99999999999795364
$ docker run -v $PWD:/workdir verificarlo/verificarlo \
   ./test
999.99999999999761258
```

### Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * GNU mpfr library http://www.mpfr.org/
  * LLVM, clang and opt from 3.3 up to 4.0.1 (the last version with Fortran support is 3.6), http://clang.llvm.org/
  * gcc, gfortran and dragonegg (for Fortran support), http://dragonegg.llvm.org/
  * python, version >= 2.7
  * autotools (automake, autoconf)

Then run the following command inside verificarlo directory:

```bash
   $ ./autogen.sh
   $ ./configure
   $ make
   $ sudo make install
```

If you do not care about Fortran support, you can avoid installing gfortran and dragonegg, by passing the option `--without-dragonegg` to `configure`:

```bash
   $ ./autogen.sh
   $ ./configure --without-dragonegg
   $ make
   $ sudo make install
```

If needed LLVM path, dragonegg path, and gcc path can be configured with the
following options:

```bash
   $ ./configure --with-llvm=<path to llvm install directory> \
                 --with-dragonegg=<path to dragonegg.so> \
                 CC=<gcc binary compatible with installed dragonegg>
```

Once installation is over, we recommend that you run the test suite to ensure
verificarlo works as expected on your system:

```bash
   $ make installcheck
```

If you disable dragonegg support during configure, fortran_test will fail.

For example on an x86_64 Ubuntu 14.04 release, you should use the following
install procedure:

```bash
   $ sudo apt-get install libmpfr-dev clang-3.3 llvm-3.3-dev dragonegg-4.7 \
       gcc-4.7 gfortran-4.7 autoconf automake build-essential

   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure \
       --with-dragonegg=/usr/lib/gcc/x86_64-linux-gnu/4.7/plugin/dragonegg.so \
       CC=gcc-4.7
   $ make 
   $ sudo make install
   $ make installcheck
```

### Usage

To automatically instrument a program with Verificarlo you must compile it using
the `verificarlo` command. First make sure that the verificarlo installation
directory is in your PATH.

Then you can use the `verificarlo` command to compile your programs. Either modify 
your makefile to use `verificarlo` as the compiler (`CC=verificarlo` and
`FC=verificarlo` ) and linker (`LD=verificarlo`) or use the verificarlo command
directly:

```bash
   $ verificarlo *.c *.f90 -o ./program
```

If you only wish to instrument a specific function in your program, use the
`--function` option:

```bash
   $ verificarlo *.c -o ./program --function=specificfunction
```

When invoked with the `--verbose` flag, verificarlo provides detailed output of
the instrumentation process.

It is important to include the necessary link flags if you use extra libraries. For example, you should include `-lm` if you are linking against the math library and include `-lstdc++` if you use functions in the standard C++ library.

### MCA Configuration Parameters

Two environement variables control the Montecarlo Arithmetic parameters.

The environement variable `VERIFICARLO_MCAMODE` controls the arithmetic error
mode. It accepts the following values:

 * `MCA`: (default mode) Montecarlo Arithmetic with inbound and outbound errors
 * `IEEE`: the program uses standard IEEE arithmetic, no errors are introduced
 * `PB`: Precision Bounding inbound errors only
 * `RR`: Random Rounding outbound errors only

The environement variable `VERIFICARLO_PRECISION` controls the virtual precision
used for the floating point operations. It accepts an integer value that
represents the virtual precision at which MCA operations are performed. Its
default value is 53. For a more precise definition of the virtual precision, you
can refer to https://hal.archives-ouvertes.fr/hal-01192668.

Verificarlo supports two MCA backends. The environement variable
`VERIFICARLO_BACKEND` is used to select the backend. It can be set to `QUAD` or
`MPFR`

The default backend, MPFR, uses the GNU multiple precision library to compute
MCA operations. It is heavily based on mcalib MPFR backend.

Verificarlo offers an alternative MCA backend: the QUAD backend. QUAD backend
uses the GCC quad types to compute MCA operations on doubles and the double type
to compute MCA operations on floats. It is much faster than the MPFR backend,
but is recent and still experimental.

One should note when using the QUAD backend, that the round operations during
MCA computation always use round-to-zero mode.

In Random Round mode, the exact operations in given virtual precision are
preserved. 

### Examples

The `tests/` directory contains various examples of Verificarlo usage.

### Postprocessing

The `postprocessing/` directory contains postprocessing tools to compute floating
point accuracy information from a set of verificarlo generated outputs.

For now we only have a VTK postprocessing tool `vfc-vtk.py` which takes multiple
VTK outputs generated with verificarlo and generates a single VTK set of files that
is enriched with accuracy information for each floating point `DataArray`.

For more information about `vfc-vtk.py`, please use the online help:

```bash
$ postprocess/vfc-vtk.py --help
```

### How to cite Verificarlo


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

### Discussion Group

For questions, feedbacks or discussions about Verificarlo you can join our group at,

https://groups.google.com/forum/#!forum/verificarlo

### License
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
