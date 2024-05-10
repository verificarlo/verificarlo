lib_LTLIBRARIES = libinterflop_hashmap.la

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

libinterflop_hashmap_la_SOURCES = \
    vfc_hashmap.c 

libinterflop_hashmap_la_CFLAGS = \
    $(LTO_FLAGS) -O3 \
    -fno-stack-protector \
    -I$(top_srcdir)/.. \
    $(WARNING_FLAGS)
libinterflop_hashmap_la_LDFLAGS = \
    $(LTO_FLAGS) -O3

libinterflop_hashmap_la_includedir = $(includedir)/
