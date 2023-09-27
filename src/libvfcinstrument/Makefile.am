lib_LTLIBRARIES = libvfcinstrument.la

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS =
endif

libvfcinstrument_la_CXXFLAGS = @LLVM_CPPFLAGS@ $(ENABLE_WARNINGS)
libvfcinstrument_la_LDFLAGS = @LLVM_LDFLAGS@
libvfcinstrument_la_SOURCES = libVFCInstrument.cpp
