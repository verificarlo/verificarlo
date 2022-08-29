lib_LTLIBRARIES = libinterflop_bitmask.la
libinterflop_bitmask_la_SOURCES = interflop_bitmask.c ../../common/logger.c ../../common/options.c
libinterflop_bitmask_la_CFLAGS = -DBACKEND_HEADER="interflop_bitmask" -O3
if WALL_CFLAGS
libinterflop_bitmask_la_CFLAGS += -Wall -Wextra
endif
libinterflop_bitmask_la_LDFLAGS = -lm
libinterflop_bitmask_la_LIBADD = ../../common/rng/libvfc_rng.la
library_includedir =$(includedir)/
