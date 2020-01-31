#
# Dockerfile for Verificarlo (github.com/verificarlo/verificarlo)
# This image includes support for Fortran and uses llvm-3.5 and gcc-4.7.
#

FROM ubuntu:16.04
MAINTAINER Pablo Oliveira <pablo.oliveira@uvsq.fr>

ENV LLVM_VERSION 3.9
ENV GCC_VERSION 5
ENV GCC_PATH /usr/lib/gcc/x86_64-linux-gnu/${GCC_VERSION}

# Retrieve dependencies
# RUN apt-get -y update && apt-get -y install --no-install-recommends software-properties-common
# RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt-get -y update && apt-get -y install --no-install-recommends \
    bash ca-certificates make git libmpfr-dev clang-${LLVM_VERSION} llvm-${LLVM_VERSION} llvm-${LLVM_VERSION}-dev \
    gcc-${GCC_VERSION} gcc-${GCC_VERSION}-plugin-dev g++-${GCC_VERSION} gfortran-${GCC_VERSION} libgfortran-${GCC_VERSION}-dev \
    autoconf automake libedit-dev libtool libz-dev \
    python3 python3-numpy python3-matplotlib binutils vim && \
    rm -rf /var/lib/apt/lists/

ENV LIBRARY_PATH ${GCC_PATH}:$(llvm-config-${LLVM_VERSION} --prefix):$LIBRARY_PATH

# Setup working directory
VOLUME /workdir
WORKDIR /workdir

# Download dragonegg from a non official repo since the official version does not
# work with llvm-3.6 and gcc-4.9
RUN git clone -b gcc-5 https://github.com/tarunprabhu/DragonEgg.git
WORKDIR DragonEgg
RUN LLVM_CONFIG=llvm-config-${LLVM_VERSION} GCC=gcc-${GCC_VERSION} GCC=g++-${GCC_VERSION} make
ENV DRAGONEGG_PATH=$PWD/dragonegg.so

WORKDIR /workdir

# Download and configure verificarlo from git master
RUN \
  git clone https://github.com/verificarlo/verificarlo.git && \
  cd verificarlo && \
  ./autogen.sh && \
  ./configure --with-llvm=$(llvm-config-${LLVM_VERSION} --prefix) --with-dragonegg=${DRAGONEGG_PATH} CC=${GCC_VERSION} CXX=g++-{GCC_VERSION}

# Build and test verificarlo

ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH
ENV PATH /usr/local/bin:$PATH
ENV PYTHONPATH /usr/local/lib/python3.4/site-packages/:$PYTHONPATH
RUN cd verificarlo && make && make install && make installcheck
