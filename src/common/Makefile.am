SUBDIRS=rng
noinst_LTLIBRARIES = libvprec_tools.la libvfc_hashmap.la
libvprec_tools_la_CFLAGS = -fPIC -static
if WALL_CFLAGS
libvprec_tools_la_CFLAGS += -Wall -Wextra
endif
libvprec_tools_la_SOURCES = \
    vprec_tools.h \
    vprec_tools.c
libvfc_hashmap_la_CFLAGS = -fPIC -static
if WALL_CFLAGS
libvfc_hashmap_la_CFLAGS += -Wall -Wextra
endif
libvfc_hashmap_la_SOURCES = \
    vfc_hashmap.h \
    vfc_hashmap.c
lib_LTLIBRARIES = libvfc_probes.la
libvfc_probes_la_SOURCES = vfc_probes.c vfc_probes.h
if WALL_CFLAGS
libvfc_probes_la_CFLAGS = -Wall -Wextra -Wno-varargs
endif
libvfc_probes_la_LIBADD = libvfc_hashmap.la
if BUILD_FLANG
lib_LIBRARIES = libvfc_probes_f.a
libvfc_probes_f_a_SOURCES = vfc_probes_f.f90
library_includedir =$(includedir)/
include_HEADERS = vfc_probes_f.mod
endif
