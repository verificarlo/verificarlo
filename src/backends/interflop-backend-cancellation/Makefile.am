ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES = libinterflop_cancellation.la libinterflop_cancellation_no-tls.la

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
libinterflop_cancellation_la_SOURCES = interflop_cancellation.c

libinterflop_cancellation_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    -fno-stack-protector \
    -DRNG_THREAD_SAFE \
    $(LTO_FLAGS) -Og \
    $(WARNING_FLAGS)

libinterflop_cancellation_la_LDLAGS = $(LTO_FLAGS) -Og

libinterflop_cancellation_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_rng.la \
    @INTERFLOP_LIBDIR@/libinterflop_fma.la \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la

# Backend version with TLS disabled
libinterflop_cancellation_no_tls_la_SOURCES = interflop_cancellation.c

libinterflop_cancellation_no_tls_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    -fno-stack-protector \
    $(LTO_FLAGS) -Og \
    $(WARNING_FLAGS)

libinterflop_cancellation_no_tls_la_LDLAGS = $(LTO_FLAGS) -Og

libinterflop_cancellation_no_tls_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_rng.la \
    @INTERFLOP_LIBDIR@/libinterflop_fma.la \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la

includesdir=$(includedir)/interflop
includes_HEADERS= interflop_cancellation.h

