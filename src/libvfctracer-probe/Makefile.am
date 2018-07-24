libvfctracer_probe_la_CXXFLAGS = @LLVM_CPPFLAGS@
libvfctracer_probe_la_LDFLAGS = @LLVM_LDFLAGS@
lib_LTLIBRARIES = libvfctracer_probe.la
libvfctracer_probe_la_SOURCES = libtracer-probe.cpp ../libvfctracer/vfctracer.cpp \
				../libvfctracer/Data/Data.cpp ../libvfctracer/Data/ScalarData.cpp \
				../libvfctracer/Data/VectorData.cpp ../libvfctracer/Format/Format.cpp \
				../libvfctracer/Data/ProbeData.cpp  \
				../libvfctracer/Format/BinaryFmt.cpp ../libvfctracer/Format/TextFmt.cpp \
				../libvfctracer/opcode.cpp
libvfctracer_probe_includedir = $(includedir)/
libvfctracer_probe_include_HEADERS = ../libvfctracer/vfctracer.hxx ../libvfctracer/Data/Data.hxx \
				     ../libvfctracer/Format/Format.hxx ../libvfctracer/opcode.hxx \
				     vfctracer-probe.h
