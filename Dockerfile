#
# Dockerfile for Verificarlo (github.com/verificarlo/verificarlo)
# This image includes support for Fortran and uses llvm-3.5 and gcc-4.7.
#

FROM ubuntu:14.04
MAINTAINER Pablo Oliveira <pablo.oliveira@uvsq.fr>

# Retrieve dependencies
RUN apt-get -y update
RUN apt-get -y install --no-install-recommends bash ca-certificates make git libmpfr-dev clang-3.5 llvm-3.5 llvm-3.5-dev dragonegg-4.7 gcc-4.7 g++-4.7 gfortran-4.7 libgfortran-4.7-dev autoconf automake libedit-dev libtool libz-dev python && rm -rf /var/lib/apt/lists/
ENV LIBRARY_PATH /usr/lib/gcc/x86_64-linux-gnu/4.7:/usr/lib/llvm-3.5/lib:$LIBRARY_PATH

# Download and configure verificarlo from git master
RUN \
  git clone https://github.com/verificarlo/verificarlo.git && \
  cd verificarlo && \
  ./autogen.sh && \
  ./configure --with-llvm=/usr/lib/llvm-3.5/bin/ --with-dragonegg=/usr/lib/gcc/x86_64-linux-gnu/4.7/plugin/dragonegg.so CC=gcc-4.7 CXX=g++-4.7

# Build and test verificarlo

ENV LD_LIBRARY_PATH /usr/local/lib:$LD_LIBRARY_PATH
ENV PATH /usr/local/bin:$PATH
RUN cd verificarlo && make && make install && make installcheck

# Setup working directory
VOLUME /workdir
WORKDIR /workdir


