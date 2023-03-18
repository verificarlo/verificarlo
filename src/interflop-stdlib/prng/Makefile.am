lib_LTLIBRARIES = libinterflop_prng.la

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

libinterflop_prng_la_SOURCES = \
    xoshiro.cxx \
    tinymt64.c 

libinterflop_prng_la_CFLAGS = \
    $(LTO_FLAGS) -O3 \
    -fno-stack-protector
libinterflop_prng_la_CPPFLAGS = \
    $(LTO_FLAGS) -O3 \
    -fno-stack-protector
libinterflop_prng_la_LDFLAGS = \
    $(LTO_FLAGS) -O3

