lib_LTLIBRARIES = libinterflop_rng.la

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

libinterflop_rng_la_SOURCES = \
    splitmix64.c \
    xoroshiro128.c \
    vfc_rng.c

libinterflop_rng_la_CFLAGS = \
    $(LTO_FLAGS) -O3 \
    -fno-stack-protector \
    -I../$(top_srcdir) \
    $(WARNING_FLAGS)
libinterflop_rng_la_LDFLAGS = \
    $(LTO_FLAGS) -O3
