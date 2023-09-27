ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES = libinterflop_bitmask.la libinterflop_bitmask_no-tls.la

if ENABLE_LTO
LTO_FLAGS = -flto
else
LTO_FLAGS =
endif

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS = 
endif

# Backend version with TLS enabled
libinterflop_bitmask_la_SOURCES = interflop_bitmask.c

libinterflop_bitmask_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    -fno-stack-protector $(LTO_FLAGS) -O3 \
    -DRNG_THREAD_SAFE \
    $(LTO_FLAGS) -O3 \
    $(WARNING_FLAGS)

libinterflop_bitmask_la_LDFLAGS = $(LTO_FLAGS) -O3 

libinterflop_bitmask_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_rng.la \
    @INTERFLOP_LIBDIR@/libinterflop_fma.la \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la

# Backend version with TLS disabled
libinterflop_bitmask_no_tls_la_SOURCES = interflop_bitmask.c

libinterflop_bitmask_no_tls_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    -fno-stack-protector $(LTO_FLAGS) -O3 \
    $(LTO_FLAGS) -O3 \
    $(WARNING_FLAGS)

libinterflop_bitmask_no_tls_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_rng.la \
    @INTERFLOP_LIBDIR@/libinterflop_fma.la \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la

includesdir=$(includedir)/interflop
includes_HEADERS= interflop_bitmask.h

