#
# Dockerfile for Verificarlo (github.com/verificarlo/verificarlo)
# This image includes support for Fortran and uses llvm-7 and gcc-7
#

ARG UBUNTU_VERSION=ubuntu:20.04
FROM ${UBUNTU_VERSION}
LABEL maintainer="verificarlo contributors <verificarlo@googlegroups.com>"

ARG PYTHON_VERSION=3.8
ARG LLVM_VERSION=7
ARG GCC_VERSION=7
ARG WITH_FLANG=flang
ARG GCC_PATH=/usr/lib/gcc/x86_64-linux-gnu/${GCC_VERSION}
ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH
ENV PATH /usr/local/bin:$PATH
ENV PYTHONPATH /usr/local/lib/python${PYTHON_VERSION}/site-packages/:${PYTHONPATH}

# Retrieve dependencies
RUN apt-get -y update && apt-get -y --no-install-recommends install tzdata

RUN export UBUNTU_VERSION=$(grep 'VERSION_ID' /etc/os-release | cut -d'=' -f2 | tr -d '"' ) && \
    if [ "$UBUNTU_VERSION" = "24.04" ]; then \
    export LIBCLANG_RT="libclang-rt-${LLVM_VERSION}-dev" ; \
    fi && \
    apt-get -y install --no-install-recommends \
    bash ca-certificates make git libmpfr-dev \
    autogen dh-autoreconf autoconf automake autotools-dev libedit-dev libtool libz-dev binutils \
    clang-${LLVM_VERSION} llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev \
    libomp5-${LLVM_VERSION} libomp-${LLVM_VERSION}-dev \
    ${LIBCLANG_RT} \
    gcc-${GCC_VERSION} g++-${GCC_VERSION} \
    gfortran-${GCC_VERSION} libgfortran-${GCC_VERSION}-dev ${WITH_FLANG} \
    python3 python3-pip python3-dev cython3 parallel && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build/

ENV LIBRARY_PATH ${GCC_PATH}:$LIBRARY_PATH

# Install other Python dependencies (not available with apt-get) via pip
RUN ln -s /usr/bin/x86_64-linux-gnu-gcc-${GCC_VERSION} /usr/bin/x86_64-linux-gnu-gcc

# Download and configure verificarlo from git master
ENV AR=gcc-ar-${GCC_VERSION}
ENV RANLIB=gcc-ranlib-${GCC_VERSION}
ENV CC=gcc-${GCC_VERSION}
ENV CXX=g++-${GCC_VERSION}
COPY . /build/verificarlo/
WORKDIR /build/verificarlo


RUN if [ "$WITH_FLANG" = "flang" ]; then \
    export FLANG_OPTION="--with-flang"; \
    else \
    export FLANG_OPTION="--without-flang"; \
    fi && \
    ./autogen.sh && \
    ./configure \
    --with-llvm=$(llvm-config-${LLVM_VERSION} --prefix) \
    $FLANG_OPTION || { cat config.log; exit 1; }

# Build verificarlo
RUN make install-interflop-stdlib
RUN make && make install

# Setup working directory
VOLUME /workdir
WORKDIR /workdir
