#
# Dockerfile for Verificarlo (github.com/verificarlo/verificarlo)
# This image is based on Ubuntu 24.04 and uses llvm-20 and gcc-13
#

ARG UBUNTU_VERSION=ubuntu:24.04
FROM ${UBUNTU_VERSION}
LABEL maintainer="verificarlo contributors <verificarlo@googlegroups.com>"

ARG PYTHON_VERSION=3.12
ARG LLVM_VERSION=20
ARG GCC_VERSION=13
ARG WITH_FLANG=flang
# ARG GCC_PATH=/usr/lib/gcc/x86_64-linux-gnu/${GCC_VERSION}
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
ENV PATH=/usr/local/bin:$PATH
ENV PYTHONPATH=/usr/local/lib/python${PYTHON_VERSION}/site-packages/:${PYTHONPATH}

# Avoid being stuck with tzdata time zone configuration
ENV DEBIAN_FRONTEND=noninteractive

# Retrieve dependencies
RUN apt-get -y update && apt-get -y --no-install-recommends install tzdata

RUN export UBUNTU_VERSION=$(grep 'VERSION_ID' /etc/os-release | cut -d'=' -f2 | tr -d '"' ) && \
    if [ "$UBUNTU_VERSION" = "24.04" ]; then \
    export LIBCLANG_RT="libclang-rt-${LLVM_VERSION}-dev" ; \
    fi && \
    apt-get -y install --no-install-recommends \
    bash ca-certificates make git libmpfr-dev \
    autogen dh-autoreconf autoconf automake autotools-dev libedit-dev libtool libz-dev bzip2 binutils \
    clang-${LLVM_VERSION} llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev \
    libomp5-${LLVM_VERSION} libomp-${LLVM_VERSION}-dev \
    ${LIBCLANG_RT} \
    gcc-${GCC_VERSION} g++-${GCC_VERSION} \
    flang-${LLVM_VERSION} \
    python3 python3-pip python3-dev cython3 parallel npm && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /build/

RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 30 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION} 30 && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 30 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 30 && \
    update-alternatives --install /usr/bin/flang flang /usr/bin/flang-${LLVM_VERSION} 30 && \
    update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-${LLVM_VERSION} 30

ENV LIBRARY_PATH ${GCC_PATH}:$LIBRARY_PATH

# install bazelisk for prism
RUN npm install -g @bazel/bazelisk

# Download and configure verificarlo from git master
COPY . /build/verificarlo/
WORKDIR /build/verificarlo


RUN if [ "$WITH_FLANG" = "flang" ]; then \
    export FLANG_OPTION="--with-flang=/usr/lib/llvm-${LLVM_VERSION}/bin/flang"; \
    else \
    export FLANG_OPTION="--without-flang"; \
    fi && \
    git submodule update --init --recursive && \
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
