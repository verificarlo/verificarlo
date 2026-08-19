## Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * LLVM, clang and opt from 17.0 up to 21.1.2, http://clang.llvm.org/
  * gcc from 7
  * autotools (automake, autoconf)
  * libtool
  * flang for Fortran support (optional)
  * python3.8 or above with the following packages (automatically installed with
    `make install`):
    * numpy (version 1.19.0 or above)
    * scipy (version 1.5.0 or above)
    * pandas
    * tables
    * GitPython
    * jinja2
    * bokeh
    * significantdigits (version 0.2.0 or above)
  * GNU parallel (only required for running the test suite)
  * bzip2 (only required for running the test suite)

Then run the following command inside verificarlo directory:

```bash
   $ ./autogen.sh
   $ ./configure --without-flang
   $ sudo make install-interflop-stdlib
   $ make
   $ sudo make install
```

### Platform-specific configuration options

If you encounter build issues on certain platforms, you can use these configuration flags:

* `--without-prism`: Disable the PRISM backend. This is useful on platforms like AArch64 where PRISM dependencies (Bazel/Highway) may fail to build.
* `--without-flang`: Disable Fortran support if flang is not available or needed.

### Example on x86_64 Ubuntu 20.04 release with Fortran support

For example on an x86_64 Ubuntu 20.04 release, you should use the following
install procedure:

```bash
   $ sudo apt-get install libmpfr-dev clang-7 flang-7 llvm-7-dev parallel bzip2\
       gcc-7 autoconf automake libtool build-essential python3 python3-pip
   $ cd verificarlo/
   $ ./autogen.sh
   $  CC=gcc-7 CXX=g++-7 ./configure --with-flang
   $ sudo make install-interflop-stdlib
   $ make
   $ sudo make install
```

### Example on AArch64 platforms

On AArch64 platforms where PRISM backend dependencies may fail to build, use the `--without-prism` flag:

```bash
   $ sudo apt-get install libmpfr-dev clang llvm-dev parallel bzip2\
       gcc autoconf automake libtool build-essential python3 python3-pip
   $ cd verificarlo/
   $ ./autogen.sh
   $ ./configure --without-prism --without-flang
   $ sudo make install-interflop-stdlib
   $ make
   $ sudo make install
```

### Installing Verificarlo using `uv` (Recommended)

`uv` is an extremely fast Python package and environment manager. You can use `uv` to manage virtual environments and install dependencies efficiently:

1. Create a virtual environment with `uv`:
   ```bash
   $ uv venv .venv
   ```

2. Activate the virtual environment:
   ```bash
   $ source .venv/bin/activate
   ```

3. Install Verificarlo and dependencies:
   ```bash
   $ uv pip install .
   ```

> [!NOTE]
> Verificarlo also supports virtual environments created using Python's standard `venv` module (`python3 -m venv env && source env/bin/activate`).

### Checking installation

Once installation is over, we recommend that you run the test suite to ensure
verificarlo works as expected on your system.

If you do not use a virtual environment, you may need to export the path of the
installed python packages. For example, for a global install, this would
resemble (edit for your installed Python version):

```bash
$ export PYTHONPATH=${PYTHONPATH}:/usr/local/lib/pythonXXX.XXX/site-packages
```

You can make the above change permanent by editing your `~/.bashrc`,
`~/.profile` or whichever configuration file is relevant for your system.

Then you can run the test suite with,

```bash
   $ make installcheck
```

If you disable flang support during configure, Fortran tests will be disabled
and considered as passing the test.
