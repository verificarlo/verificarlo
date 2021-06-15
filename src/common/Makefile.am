noinst_LTLIBRARIES = libtinymt64.la libvprec_tools.la libvfc_hashmap.la

libtinymt64_la_CFLAGS = -fPIC -static
libtinymt64_la_SOURCES = \
	tinymt64.h \
	tinymt64.c

libvprec_tools_la_CFLAGS = -fPIC -static
libvprec_tools_la_SOURCES = \
	vprec_tools.h \
	vprec_tools.c

libvfc_hashmap_la_CFLAGS = -fPIC -static
libvfc_hashmap_la_SOURCES = \
	vfc_hashmap.h \
	vfc_hashmap.c


lib_LTLIBRARIES = libvfc_probes.la

libvfc_probes_la_SOURCES = vfc_hashmap.c vfc_probes.c
if WALL_CFLAGS
libvfc_probes_la_CFLAGS = -Wall -Wextra
endif

if BUILD_FLANG
lib_LIBRARIES = libvfc_probes_f.a
libvfc_probes_f_a_SOURCES = vfc_probes_f.f90
library_includedir =$(includedir)/
include_HEADERS = vfc_probes_f.mod
endif
