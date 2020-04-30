#
# Dockerfile for Verificarlo (github.com/verificarlo/verificarlo)
# This image includes support for Fortran and uses llvm-3.6.1 and gcc-4.9.
#

FROM ubuntu:16.04
MAINTAINER Verificarlo contributors <verificarlo@googlegroups.com>

ARG PYTHON_VERSION=3.5
ARG LLVM_VERSION=3.6.1
ARG GCC_VERSION=4.9
ARG GCC_PATH=/usr/lib/gcc/x86_64-linux-gnu/${GCC_VERSION}
ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH
ENV PATH /usr/local/bin:$PATH
ENV PYTHONPATH=/usr/local/lib/python$PYTHON_VERSION/site-packages/:$PYTHONPATH

# Retrieve dependencies
RUN apt-get -y update && apt-get -y install --no-install-recommends \
    bash ca-certificates make git libmpfr-dev \
    gcc-${GCC_VERSION} gcc-${GCC_VERSION}-plugin-dev g++-${GCC_VERSION} gfortran-${GCC_VERSION} libgfortran-${GCC_VERSION}-dev \
    autoconf automake libedit-dev libtool libz-dev \
    python3 python3-dev python3-numpy python3-matplotlib python3-pip python3-setuptools binutils vim sudo wget xz-utils && \
    rm -rf /var/lib/apt/lists/

WORKDIR /build/

# Download llvm-3.6.1 since it does not work with the version llvm-3.6.2 provided by apt
RUN wget http://releases.llvm.org/3.6.1/clang+llvm-3.6.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz && \
    tar xvf clang+llvm-3.6.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz && \
    ln -s clang+llvm-3.6.1-x86_64-linux-gnu llvm-3.6.1

ENV LLVM_INSTALL_PATH /build/llvm-3.6.1
ENV LIBRARY_PATH ${GCC_PATH}:${LLVM_INSTALL_PATH}:$LIBRARY_PATH

# Download dragonegg from a non official repo since the official
# version does not work with llvm-3.6.1 and gcc-4.9. gcc-4.9 and
# higher version is required for using _Generic feature.  This is
# temporary since we are planning to move to flang and do not use
# dragonegg anymore.
RUN git clone -b gcc-llvm-3.6 --depth=1 https://github.com/yohanchatelain/DragonEgg.git
WORKDIR DragonEgg
RUN LLVM_CONFIG=${LLVM_INSTALL_PATH}/bin/llvm-config GCC=gcc-${GCC_VERSION} CXX=g++-${GCC_VERSION} make
ARG DRAGONEGG_PATH=/build/DragonEgg/dragonegg.so

# Install bigfloat package for manipulating MPFR with Python
RUN ln -s /usr/bin/x86_64-linux-gnu-gcc-4.9 /usr/bin/x86_64-linux-gnu-gcc && \
    sudo pip3 install --upgrade pip && \
    sudo pip3 install bigfloat

# Download and configure verificarlo from git master
WORKDIR /build/verificarlo
RUN git clone --depth=1 https://github.com/verificarlo/verificarlo.git &&  \
    cd verificarlo && \
    ./autogen.sh && \
    ./configure --with-llvm=${LLVM_INSTALL_PATH} --with-dragonegg=${DRAGONEGG_PATH} CC=gcc-${GCC_VERSION} CXX=g++-${GCC_VERSION} || cat config.log 

# Build and test verificarlo
RUN cd verificarlo && \
    make && sudo make install && \
    make installcheck

# Setup working directory
VOLUME /workdir
WORKDIR /workdir
