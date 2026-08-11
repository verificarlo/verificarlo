ACLOCAL_AMFLAGS=-I m4
lib_LTLIBRARIES = libinterflop_vfcwrapper.la

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

libinterflop_vfcwrapper_la_SOURCES = main.c backends.c funcinstr.c
noinst_HEADERS = \
    backends.h \
    funcinstr.h \
    operation_amd.h \
    operation_arm.h \
    operation_x86.h \
    operator.h \
    vector_ext.h

libinterflop_vfcwrapper_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    -fno-stack-protector $(LTO_FLAGS) -O3 \
    $(LTO_FLAGS) -O3 \
    $(WARNING_FLAGS)

libinterflop_vfcwrapper_la_LDFLAGS = -O3 -ldl

libinterflop_vfcwrapper_la_LIBADD = \
    @INTERFLOP_LIBDIR@/libinterflop_logger.la \
    @INTERFLOP_LIBDIR@/libinterflop_stdlib.la \
    @INTERFLOP_LIBDIR@/libinterflop_hashmap.la 

include_HEADERS = vfcwrapper.h
