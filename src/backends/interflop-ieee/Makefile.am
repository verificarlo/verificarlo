lib_LTLIBRARIES = libinterflop_ieee.la
libinterflop_ieee_la_SOURCES = interflop_ieee.c ../../common/printf_specifier.c ../../common/logger.c
libinterflop_ieee_la_CFLAGS = -DBACKEND_HEADER="interflop_ieee"
if WALL_CFLAGS
libinterflop_ieee_la_CFLAGS += -Wall -Wextra
endif
libinterflop_ieee_la_LDFLAGS = -lm
library_includedir =$(includedir)/
