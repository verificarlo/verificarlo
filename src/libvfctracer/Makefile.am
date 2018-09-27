libvfctracer_la_CXXFLAGS = @LLVM_CPPFLAGS@
libvfctracer_la_LDFLAGS = @LLVM_LDFLAGS@
EXTRA_DIST = LocationInfo.hxx RangeID.hxx vfctracer.hxx Data/Data.hxx Format/Format.hxx
lib_LTLIBRARIES = libvfctracer.la
libvfctracer_la_SOURCES = libtracer.cpp vfctracer.cpp RangeID.cpp LocationInfo.cpp  \
	Data/Data.cpp Data/ScalarData.cpp Data/VectorData.cpp Data/ProbeData.cpp Data/PredicateData.cpp \
	Format/Format.cpp Format/BinaryFmt.cpp Format/TextFmt.cpp \
	opcode.cpp 
libvfctracer_includedir = $(includedir)/
libvfctracer_include_HEADERS = LocationInfo.hxx RangeID.hxx vfctracer.hxx Data/Data.hxx Format/Format.hxx opcode.hxx 
