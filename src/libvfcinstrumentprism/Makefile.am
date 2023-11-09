lib_LTLIBRARIES = libvfcinstrumentprism.la  

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS =
endif

libvfcinstrumentprism_la_CXXFLAGS = @LLVM_CPPFLAGS@ -I@INTERFLOP_INCLUDEDIR@ -Wfatal-errors 
libvfcinstrumentprism_la_LDFLAGS = @LLVM_LDFLAGS@
libvfcinstrumentprism_la_SOURCES = libVFCInstrumentPRISM.cpp TargetFeatures.hpp libVFCInstrumentPRISMOptions.hpp