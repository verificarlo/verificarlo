lib_LTLIBRARIES = libinterflop_vprec.la
libinterflop_vprec_la_SOURCES = interflop_vprec.c  ../../common/logger.c
libinterflop_vprec_la_CFLAGS = -DBACKEND_HEADER="interflop_vprec"
if WALL_CFLAGS
libinterflop_vprec_la_CFLAGS += -Wall -Wextra -Wno-varargs
endif
libinterflop_vprec_la_LDFLAGS = -lm
libinterflop_vprec_la_LIBADD = ../../common/libvprec_tools.la ../../common/libvfc_hashmap.la
library_includedir =$(includedir)/
