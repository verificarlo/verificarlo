<img src="https://avatars1.githubusercontent.com/u/12033642" align="right" height="200px" \>

## Veritracer v0.0.1

[![Build Status](https://travis-ci.org/verificarlo/verificarlo.svg?branch=master)](https://travis-ci.org/verificarlo/verificarlo)

VeriTracer, a visualization tool that brings temporal dimension to a graphical Floating-Point analysis.

### Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * GNU mpfr library http://www.mpfr.org/
  * LLVM, clang and opt from 3.3 up to 3.8 (the last version with Fortran support is 3.6), http://clang.llvm.org/
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

To automatically trace a program with Veritracer, you must compile it using
the `verificarlo --tracer` command.
First, make sure that the verificarlo installation
directory is in your PATH.

Then you can use the `verificarlo --tracer` command to compile your programs. Either modify 
your makefile to use `verificarlo` as the compiler (`CC=verificarlo` and
`FC=verificarlo` ) and linker (`LD=verificarlo`) or use the verificarlo command
directly:

```bash
   $ verificarlo --tracer *.c *.f90 -o ./program
```

If you only wish to instrument a specific function in your program, use the
`--function` option:

```bash
   $ verificarlo --tracer *.c -o ./program --function=specificfunction
```

When invoked with the `--verbose` flag, veritracer provides detailed output of
the instrumentation process. 

Veritracer builds a file containing all information gathered during the compilation
of variables instrumented. Information is collected in the `locationInfo.map` file.
By default, this file is created in the directory where compilation is made.
You can change it by modifying the environment variable `VERITRACER_LOCINFO_PATH`.

After execution, veritracer produces a file named `range_tracer.dat` which contain the raw
values collected during the execution. By default, it is a binary file,
but it can be switched to text format by specifying the `--tracer-format=text`
option.

Tools are provided for processing traces. However, for gathering
data with the script, you must respect the following format for your directory
which is explained in the Postprocessing section.

### Postprocessing

The  `postprocessing/veritracer/` directory contains postprocessing tools
for visualizing information produced by veritracer. It exists two tools
`veritracer_analyzer.py` and `veritracer_plot.py` which respectively
allow gathering and visualizing information. 

```bash
   $ veritracer_analyzer.py
```
For gathering data with the script,
you must respect the following format for your directory:
for `n` runs in the directory `exp`,
you should have:

```bash
   $ ls -R exp
   $ exp/1: tracer.dat exp/2: tracer.dat ... exp/n: tracer.dat
   $
   $ cd exp/
   $ veritracer_analyzer.py -f tracer.dat -o output.csv
```

Veritracer_plot.py allows visualizing data from `output.csv` file.


```bash
   $ veritracer_plot.py -f file.csv 
```

For visualizing specific variables, you can use the `--variables` option.
Use the hash value of the variable which is available in the `locationInfo.map`.

```bash
   $ veritracer_plot.py -f file.csv --variables=<hash1> <hash2> ... <hashN>
```

### Examples

The `tests/veritracer` directory contains an example of Veritracer usage.

![](simp_gen.jpg)

`veritracer_plot.py` usage on ABINIT code.  

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

Copyright (c) 2017
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
