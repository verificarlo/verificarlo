## Verificarlo

[![Build Status](https://travis-ci.org/verificarlo/verificarlo.svg?branch=master)](https://travis-ci.org/verificarlo/verificarlo)

A tool for automatic Montecarlo Arithmetic analysis.

### Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * GNU mpfr library http://www.mpfr.org/
  * LLVM, clang and opt 3.3 or 3.4, http://clang.llvm.org/
  * gcc, gfortran and dragonegg (for Fortran support), http://dragonegg.llvm.org/
  * python, version >= 2.7
  * autotools (automake, autoconf)

Then run the following command inside verificarlo directory:

```bash
   $ ./autogen.sh
   $ ./configure
   $ make
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
   $ make check
```


For example on an x86_64 Ubuntu 14.04 release, you should use the following
install procedure:

```bash
   $ sudo apt-get install libmpfr-dev clang-3.3 llvm3.3-dev dragonegg-4.7 \
       gcc-4.7 gfortran-4.7 autoconf automake build-essential

   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure \
       --with-dragonegg=/usr/lib/gcc/x86_64-linux-gnu/4.7/plugin/dragonegg.so \
       CC=gcc-4.7
   $ make && make check
```

### Usage

To automatically instrument a program with Verificarlo you must compile it using
the `verificarlo` command. First make sure to add the `verificarlo/` root
directory to your path with

```bash
   $ export PATH=<verificarlo path>:$PATH
```

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

### MCA Configuration Parameters

Two environement variables control the Montecarlo Arithmetic parameters.

The environement variable `VERIFICARLO_MCAMODE` controls the arithmetic error
mode. It accepts the following values:

 * `MCA`: (default mode) Montecarlo Arithmetic with inbound and outbound errors
 * `IEEE`: the program uses standard IEEE arithmetic, no errors are introduced
 * `PB`: Precision Bounding inbound errors only
 * `RR`: Random Rounding outbound errors only

The environement variable `VERIFICARLO_PRECISION` controls the virtual
precision used for the floating point operations. It accept an integer value
that represents the number of significant digits. The default value is 53.

### Examples

The `tests/` directory contains various examples of Verificarlo usage.

### License

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
