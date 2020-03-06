lib_LTLIBRARIES = libinterflop_mca_mpfr.la
libinterflop_mca_mpfr_la_SOURCES = interflop_mca_mpfr.c ../../common/logger.c ../../common/options.c
libinterflop_mca_mpfr_la_CFLAGS = -DBACKEND_HEADER="interflop_mca_mpfr"
if WALL_CFLAGS
libinterflop_mca_mpfr_la_CFLAGS += -Wall -Wextra
endif
libinterflop_mca_mpfr_la_LDFLAGS = -lm -lmpfr
libinterflop_mca_mpfr_la_LIBADD = ../../common/libtinymt64.la
library_includedir =$(includedir)/
