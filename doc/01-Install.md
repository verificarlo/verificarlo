## Installation

Please ensure that Verificarlo's dependencies are installed on your system:

  * LLVM, clang and opt from 17.0 up to 20.1.2, http://clang.llvm.org/
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

### Installing Verificarlo in an virtual Python environment (venv)

If you want to isolate your Python environment for Verificarlo, you can use
`venv`, which is a module that comes with Python 3. It creates a virtual
environment that has its own installation directories and doesn't share
libraries with other virtual environments.

Here's how you can create and activate a virtual environment:

1. First, navigate to the directory where you want to create the virtual
   environment. Then, run the following command to create a new virtual
   environment. Replace `env` with the name you want to give to your virtual
   environment:

   ```bash
   $ python3 -m venv env
   ```

   This command creates a new directory named `env` (or whatever name you gave)
   which contains the directories for your virtual environment.

2. To activate the virtual environment, run the following command:

      ```bash
      $ source env/bin/activate
      ```

   When the virtual environment is activated, the name of your virtual
   environment will appear on the left of the prompt to let you know that it’s
   active. From now on, any package that you install using pip will be placed in
   the `env` folder, isolated from the global Python installation.

3. If you want to deactivate the virtual environment and use your original
   Python environment, simply run:

   ```bash
   $ deactivate
   ```

You can install Verificarlo in this isolated environment. This ensures that the
Verificarlo installation doesn't interfere with your global Python environment.

To install Verificarlo in the virtual environment, you need to use the
`--prefix` option with the `./configure` command. The `--prefix` option should
point to the path of your virtual environment. You can use the `VIRTUAL_ENV`
environment variable, which is set when you activate the virtual environment, to
get this path.

Here's how you can install Verificarlo in the virtual environment:

```bash
$ source env/bin/activate  # Activate the virtual environment
$ cd verificarlo/
$ ./autogen.sh
$ CC=gcc-7 CXX=g++-7 ./configure --with-flang --prefix=$VIRTUAL_ENV
$ make
$ make install
```

In this example, `--prefix=$VIRTUAL_ENV` tells `./configure` to install
Verificarlo in the `env` directory (or whatever name you gave to your virtual
environment). This means that the Verificarlo binaries will be placed in the
`bin` directory of your virtual environment, and the libraries will be placed in
the `lib` directory of your virtual environment.

Remember to activate the virtual environment before you run these commands. This
ensures that you are using the Python interpreter and libraries in the virtual
environment, and keeps your global environment clean and isolated.

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
