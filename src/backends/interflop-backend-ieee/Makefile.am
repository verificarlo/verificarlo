ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES = libinterflop_ieee.la

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

libinterflop_ieee_la_SOURCES = \
    interflop_ieee.c \
    common/printf_specifier.c

libinterflop_ieee_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@/ \
    -fno-stack-protector \
    $(LTO_FLAGS) -O3 \
    $(WARNING_FLAGS)

libinterflop_ieee_la_LDLAGS = $(LTO_FLAGS) -O3

libinterflop_ieee_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_fma.la \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la

includesdir=$(includedir)/interflop
includes_HEADERS= interflop_ieee.h
