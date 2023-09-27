lib_LTLIBRARIES = libvfcfuncinstrument.la

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS =
endif

libvfcfuncinstrument_la_CXXFLAGS = @LLVM_CPPFLAGS@ $(ENABLE_WARNINGS)
libvfcfuncinstrument_la_LDFLAGS = @LLVM_LDFLAGS@
libvfcfuncinstrument_la_SOURCES = libVFCFuncInstrument.cpp