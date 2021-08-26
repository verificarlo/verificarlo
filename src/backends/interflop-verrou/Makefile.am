lib_LTLIBRARIES = libinterflop_verrou.la
libinterflop_verrou_la_SOURCES = interflop_verrou.cxx
libinterflop_verrou_la_CFLAGS = -DBACKEND_HEADER="interflop_verrou" -DINTERFLOP_DYN_INTERFACE_ENABLED
if WALL_CFLAGS
libinterflop_verrou_la_CFLAGS += -Wall -Wextra
endif
libinterflop_verrou_la_LDFLAGS = -lm
library_includedir =$(includedir)/
