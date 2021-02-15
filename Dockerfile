#
# Dockerfile for Verificarlo (github.com/verificarlo/verificarlo)
# This image includes support for Fortran and uses llvm-7 and gcc-7
#

FROM ubuntu:20.04
LABEL maintainer="verificarlo contributors <verificarlo@googlegroups.com>"

ARG PYTHON_VERSION=3.8
ARG LLVM_VERSION=7
ARG GCC_VERSION=7
ARG GCC_PATH=/usr/lib/gcc/x86_64-linux-gnu/${GCC_VERSION}
ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH
ENV PATH /usr/local/bin:$PATH
ENV PYTHONPATH /usr/local/lib/python$PYTHON_VERSION/site-packages/:$PYTHONPATH

# Retrieve dependencies
RUN apt-get -y update && apt-get -y --no-install-recommends install tzdata 
RUN apt-get -y install --no-install-recommends \
    bash ca-certificates make git libmpfr-dev \
    autogen dh-autoreconf autoconf automake autotools-dev libedit-dev libtool libz-dev binutils \
    clang-${LLVM_VERSION} llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev \
    gcc-${GCC_VERSION} g++-${GCC_VERSION} \
    gfortran-${GCC_VERSION} libgfortran-${GCC_VERSION}-dev flang \
    python3 python3-pip python3-numpy python3-matplotlib python3-dev cython3 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build/

ENV LIBRARY_PATH ${GCC_PATH}:$LIBRARY_PATH

# Install bigfloat package for manipulating MPFR with Python
RUN ln -s /usr/bin/x86_64-linux-gnu-gcc-7 /usr/bin/x86_64-linux-gnu-gcc && \
    pip3 install --upgrade pip && \
    pip3 install bigfloat

# Download and configure verificarlo from git master
COPY . /build/verificarlo/
WORKDIR /build/verificarlo
RUN ./autogen.sh && \
    ./configure --with-llvm=$(llvm-config-${LLVM_VERSION} --prefix) \
    --with-flang CC=gcc-${GCC_VERSION} CXX=g++-${GCC_VERSION} \
    || cat config.log

# Build verificarlo
RUN make && make install

# Setup working directory
VOLUME /workdir
WORKDIR /workdir
