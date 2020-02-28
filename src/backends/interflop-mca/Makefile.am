lib_LTLIBRARIES = libinterflop_mca.la
libinterflop_mca_la_SOURCES = interflop_mca.c ../../common/logger.c ../../common/options.c
libinterflop_mca_la_CFLAGS = -DBACKEND_HEADER="interflop_mca"
if WALL_CFLAGS
libinterflop_mca_la_CFLAGS += -Wall -Wextra
endif
libinterflop_mca_la_LDFLAGS = -lm
libinterflop_mca_la_LIBADD = ../../common/libtinymt64.la
library_includedir =$(includedir)/
