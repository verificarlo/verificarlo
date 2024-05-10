ACLOCAL_AMFLAGS= -I m4
SUBDIRS=rng fma hashmap iostream
lib_LTLIBRARIES = libinterflop_stdlib.la

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

libinterflop_stdlib_la_SOURCES = \
	interflop_stdlib.h \
	interflop_stdlib.c

libinterflop_stdlib_la_CFLAGS = \
	$(LTO_FLAGS) -O3 \
	-fno-stack-protector \
	$(WARNING_FLAGS)
libinterflop_stdlib_la_LDFLAGS = \
	$(LTO_FLAGS) -O3

headersdir=$(includedir)/interflop
headers_HEADERS=interflop.h interflop_stdlib.h
nobase_headers_HEADERS= \
	iostream/logger.h \
	rng/vfc_rng.h \
	rng/xoroshiro128.h \
	fma/interflop_fma.h \
	common/float_const.h \
	common/float_struct.h \
	common/float_utils.h \
	common/generic_builtin.h \
	common/options.h \
	hashmap/vfc_hashmap.h

m4dir = $(datarootdir)/interflop
m4_DATA = \
	m4/ax_interflop_stdlib.m4 \
	m4/ax_warnings.m4 \
	m4/ax_lto.m4 \
	m4/ax_interflop_rng.m4

