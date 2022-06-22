## Verificarlo v0.7.0

A tool for debugging and assessing floating point precision and reproducibility.

![verificarlo logo](https://avatars1.githubusercontent.com/u/12033642)

![Build Status](https://github.com/verificarlo/verificarlo/workflows/test-docker/badge.svg?branch=master)
[![Docker Pulls](https://img.shields.io/docker/pulls/verificarlo/verificarlo)](https://hub.docker.com/r/verificarlo/verificarlo)
[![DOI](https://zenodo.org/badge/34260221.svg)](https://zenodo.org/badge/latestdoi/34260221)
[![Coverity](https://scan.coverity.com/projects/19956/badge.svg)](https://scan.coverity.com/projects/verificarlo-verificarlo)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://releases.llvm.org/13.0.0/LICENSE.TXT)


   * [Installation](#installation)
   * [Using Verificarlo through its Docker image](#using-verificarlo-through-its-docker-image)
   * [Usage](#usage)
   * [Branch instrumentation](#branch-instrumentation)
   * [Examples and Tutorial](#examples-and-tutorial)
   * [Backends](#backends)
   * [Inclusion / exclusion options](#inclusion--exclusion-options)
   * [Pinpointing numerical errors with Delta-Debug](#pinpointing-numerical-errors-with-delta-debug)
   * [VPREC Function instrumentation](#vprec-function-instrumentation)
   * [User call instrumentation](#interflop-usercall-instrumentation)
   * [Postprocessing](#postprocessing)
   * [How to cite Verificarlo](#how-to-cite-verificarlo)
   * [Discussion Group](#discussion-group)
   * [License](#license)


## Installation

To install Verificarlo please refer to the [installation documentation](doc/01-Install.md).

## Using Verificarlo through its Docker image

A docker image is available at https://hub.docker.com/r/verificarlo/verificarlo/.
This image uses the latest release version of Verificarlo and includes
support for Fortran. It uses llvm-7 and gcc-7.

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
   verificarlo-c test.c -o test
$ docker run -v "$PWD":/workdir -e VFC_BACKENDS="libinterflop_mca.so" \
   verificarlo/verificarlo ./test
999.99999999999795364
$ docker run -v "$PWD":/workdir -e VFC_BACKENDS="libinterflop_mca.so" \
   verificarlo/verificarlo ./test
999.99999999999761258
```

## Usage

To automatically instrument a program with Verificarlo you must compile it using
the `verificarlo --linker=<linker>` command, where `<linker>` depends on the targeted language:


* `verificarlo --linker=clang`   for C
* `verificarlo --linker=clang++` for C++
* `verificarlo --linker=flang`   for Fortran

Verificarlo uses the linker `clang` by default.

You can also use the provided wrappers to call `verificarlo` with the right linker:

* `verificarlo-c` for C
* `verificarlo-c++` for C++
* `verificarlo-f` for Fortran

First make sure that the verificarlo installation
directory is in your PATH.

Then you can use the `verificarlo-c`, `verificarlo-f` and `verificarlo-c++` commands to compile your programs.
Either modify your makefile to use `verificarlo` as the compiler (`CC=verificarlo-c`,
`FC=verificarlo-f` and `CXX=verificarlo-c++`) and linker (`LD=verificarlo --linker=<linker>`) or use the verificarlo command
directly:

```bash
   $ verificarlo-c program.c -o ./program
```

If you are trying to compile a shared library, such as those built by the Cython
extension to Python, you can then also set the shared linker environment variable
(`LDSHARED='verificarlo --linker=<linker> -shared'`) to enable position-independent linking.

When invoked with the `--verbose` flag, verificarlo provides detailed output of
the instrumentation process.

It is important to include the necessary link flags if you use extra libraries.
For example, you should include `-lm` if you are linking against the math
library.

## Branch instrumentation

Verificarlo can instrument floating point comparison operations. By default,
comparison operations are not instrumented and default backends do not make use of
this feature. If your backend requires instrumenting floating point comparisons, you
must call `verificarlo` with the `--inst-fcmp` flag.

## Examples and Tutorial

The `tests/` directory contains various examples of Verificarlo usage.

A [tutorial](https://github.com/verificarlo/verificarlo/wiki/Tutorials) is available.

## Backends

Verificarlo includes different numerical backends. Please refer to the [backends documentation](doc/02-Backends.md).

  * [IEEE Backend (libinterflop_ieee.so)](doc/02-Backends.md#ieee-backend-libinterflop_ieeeso)
  * [MCA Backends (libinterflop_mca.so and lib_interflop_mca_int.so)](doc/02-Backends.md#mca-backends)
  * [Bitmask Backend (libinterflop_bitmask.so)](doc/02-Backends.md#bitmask-backend-libinterflop_bitmaskso)
  * [Cancellation Backend (libinterflop_cancellation.so)](doc/02-Backends.md#cancellation-backend-libinterflop_cancellationso)
  * [VPREC Backend (libinterflop_vprec.so)](doc/02-Backends.md#vprec-backend-libinterflop_vprecso)

## Inclusion / exclusion options

To inlude only certain parts of the code in the analysis or exclude parts of
the code from instrumentation please refer to [inclusion / exclusion options documentation](doc/03-inclusion-exclusion.md).


## Pinpointing numerical errors with Delta-Debug

To pinpoint numerical errors please refer to the [Delta-Debug documentation](doc/04-DeltaDebug.md).

## VPREC Function Instrumentation

A function instrumentation pass enables VPREC exploration and optimization at
the function granularity level. Please refer to the [VPREC Function Instrumentation documentation](doc/05-VPREC-function-instrumentation.md).

## Postprocessing

Verificarlo includes a set of [postprocessing tools](doc/06-Postprocessing.md) to help analyse Verificarlo results and produce high-level reports.

  * [Find Optimal precision with vfc_precexp and vfc_report](doc/06-Postprocessing.md#find-optimal-precision-with-vfc_precexp-and-vfc_report)
  * [Unstable branch detection](doc/06-Postprocessing.md#unstable-branch-detection)
  * [VFC-VTK](doc/06-Postprocessing.md#vfc-vtk)
  * [Verificarlo CI](doc/06-Postprocessing.md#verificarlo-ci)

## Interflop user call instrumentation

Verificarlo provides the ability to call low-level backend functions directly through 
the `interflop_call` function. Please refer to the [Interflop user call instrumentation documentation](doc/07-Interflop-usercall-instrumentation.md).

## How to cite Verificarlo

If you use Verificarlo in your research, please cite one of the following papers:

- Verificarlo: http://dx.doi.org/10.1109/ARITH.2016.31, preprint available [here](https://hal.archives-ouvertes.fr/hal-01192668/file/verificarlo-preprint.pdf).
- VPREC Backend: https://hal.archives-ouvertes.fr/hal-02564972
- VPREC Function Instrumentation and exploration: http://dx.doi.org/10.1109/TETC.2021.3070422

Thanks !

## Discussion Group

For questions, feedbacks or discussions about Verificarlo you can use the [Discussions section](https://github.com/verificarlo/verificarlo/discussions) in our github project page.

## License
This file is part of the Verificarlo project,                        
under the Apache License v2.0 with LLVM Exceptions.                 
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.             
See https://llvm.org/LICENSE.txt for license information.            

Copyright (c) 2019-2022
   Verificarlo Contributors

Copyright (c) 2018
   Universite de Versailles St-Quentin-en-Yvelines

Copyright (c) 2015
   Universite de Versailles St-Quentin-en-Yvelines
   CMLA, Ecole Normale Superieure de Cachan
