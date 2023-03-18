ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES = libinterflop_vprec.la

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

libinterflop_vprec_la_SOURCES = \
    interflop_vprec.c \
    common/vprec_tools.c \
    interflop_vprec_function_instrumentation.c

libinterflop_vprec_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    -fno-stack-protector \
    $(LTO_FLAGS) -O3 \
    $(WARNING_FLAGS)

libinterflop_vprec_la_LDLAGS = $(LTO_FLAGS) -O3

libinterflop_vprec_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_fma.la \
    @INTERFLOP_LIBDIR@/libinterflop_hashmap.la \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la

includesdir=$(includedir)/interflop
nobase_includes_HEADERS= \
    interflop_vprec.h \
    interflop_vprec_function_instrumentation.h \
    common/vprec_tools.h
