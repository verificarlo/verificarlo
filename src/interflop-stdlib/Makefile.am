ACLOCAL_AMFLAGS= -I m4
SUBDIRS=rng prng fma hashmap iostream
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
	prng/vr_rand.h \
	prng/tinymt64.h \
	prng/xoshiro.hxx \
	fma/interflop_fma.h \
	fma/vr_fma.hxx \
	common/float_const.h \
	common/float_struct.h \
	common/float_utils.h \
	common/generic_builtin.h \
	common/quadmath-imp.h \
	common/options.h \
	hashmap/vfc_hashmap.h

m4dir = $(datarootdir)/interflop
m4_DATA = \
	m4/ax_interflop_stdlib.m4 \
	m4/ax_warnings.m4 \
	m4/ax_lto.m4 \
	m4/ax_intrinsic_fma.m4 \
	m4/ax_interflop_rng.m4

dist_bin_SCRIPTS = interflop-config

interflop-config: interflop-config.in
	./automake-expand --config-log config.log --input interflop-config.in --output interflop-config && \
	chmod +x interflop-config