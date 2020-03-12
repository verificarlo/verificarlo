lib_LTLIBRARIES = libinterflop_cancellation.la
libinterflop_cancellation_la_SOURCES = interflop_cancellation.c ../../common/logger.c ../../common/options.c
libinterflop_cancellation_la_CFLAGS = -DBACKEND_HEADER="interflop_cancellation"
if WALL_CFLAGS
libinterflop_cancellation_la_CFLAGS += -Wall -Wextra
endif
libinterflop_cancellation_la_LDFLAGS = -lm
libinterflop_cancellation_la_LIBADD = ../../common/libtinymt64.la
library_includedir =$(includedir)/
