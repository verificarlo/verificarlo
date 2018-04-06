# SUBDIRS=Data Format
libvfctracer_la_CXXFLAGS = @LLVM_CPPFLAGS@
libvfctracer_la_LDFLAGS = @LLVM_LDFLAGS@
EXTRA_DIST = vfctracer.hxx Data/Data.hxx Format/Format.hxx	
lib_LTLIBRARIES = libvfctracer.la
libvfctracer_la_SOURCES = libtracer.cpp vfctracer.cpp Data/Data.cpp Data/ScalarData.cpp Data/VectorData.cpp Format/Format.cpp Format/BinaryFmt.cpp Format/TextFmt.cpp opcode.cpp
libvfctracer_includedir = $(includedir)/
libvfctracer_include_HEADERS = vfctracer.hxx Data/Data.hxx Format/Format.hxx opcode.hxx
