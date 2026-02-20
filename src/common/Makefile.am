lib_LTLIBRARIES = libvfc_probes.la
libvfc_probes_la_SOURCES = vfc_probes.c vfc_probes.h

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS =
endif

libvfc_probes_la_CFLAGS = \
    -I@INTERFLOP_INCLUDEDIR@ \
    $(WARNING_FLAGS)

libvfc_probes_la_LIBADD = @INTERFLOP_LIBDIR@/libinterflop_hashmap.la 

if BUILD_FLANG
lib_LIBRARIES = libvfc_probes_f.a
libvfc_probes_f_a_SOURCES = vfc_probes_f.f90
include_HEADERS = vfc_probes_f.mod

vfc_probes_f.mod: vfc_probes_f.f90
	$(FC) -c $<
endif
