## Verificarlo

A tool for automatic Montecarlo Arithmetic analysis.

### Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * mcalib, https://github.com/mfrechtling/mcalib
  * clang 3.3 or 3.4, http://clang.llvm.org/
  * gcc and dragonegg (for Fortran support), http://dragonegg.llvm.org/
  * python, version >= 2.4
  * autotools (automake, autoconf)

Then run the following command inside verificarlo directory:

```bash
   $ ./autogen.sh
   $ ./configure
   $ make
```

We recommend that you run the test suite to ensure verificarlo works as expected
on your system:

```bash
   $ make check
```

### Usage

To automatically instrument a program with Verificarlo you must compile it using
the `verificarlo` command. First make sure to add the `verificarlo\` root
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

### Examples

The `tests/` directory contains various examples of Verificarlo usage.
