## Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * LLVM, clang and opt from 4.0 up to 11.0.1, http://clang.llvm.org/
  * gcc from 4.9
  * autotools (automake, autoconf)
  * flang for Fortran support (optional)
  * python3 with the following packages :
    * numpy
    * scipy (version 1.5.0 or above)
    * bigfloat
    * pandas
    * tables
    * GitPython
    * jinja2
    * bokeh
  * GNU parallel (only required for running the test suite)

Then run the following command inside verificarlo directory:

```bash
   $ ./autogen.sh
   $ ./configure --without-flang
   $ make
   $ sudo make install
```

### Example on x86_64 Ubuntu 20.04 release with Fortran support

For example on an x86_64 Ubuntu 20.04 release, you should use the following
install procedure:

```bash
   $ sudo apt-get install libmpfr-dev clang-7 flang-7 llvm-7-dev \
       gcc-7 autoconf automake libtool build-essential python3 python3-numpy \
       python3-pip
   $ sudo pip3 install bigfloat pandas scipy GitPython tables jinja2 bokeh
   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure --with-flang CC=gcc-7 CXX=g++-7
   $ make
   $ sudo make install
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

If you disable flang support during configure, Fortran tests will be
disabled and considered as passing the test.
