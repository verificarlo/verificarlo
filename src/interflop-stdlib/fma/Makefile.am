lib_LTLIBRARIES = libinterflop_fma.la

if ENABLE_LTO
LTO_FLAGS = -flto
else
LTO_FLAGS =
endif

if ENABLE_WARNINGS
WARNING_FLAGS = -Wall -Wextra -Wno-varargs
else
WARNING_FLAGS = 
endif

if HAVE_QUADMATH
QUADMATH_FLAGS = -DHAS_QUADMATH
else
QUADMATH_FLAGS =
endif


libinterflop_fma_la_SOURCES = \
    interflop_fma.c
libinterflop_fma_la_CFLAGS = \
    $(LTO_FLAGS) -O3 \
    -fno-stack-protector \
    -I../$(top_srcdir) \
    $(WARNING_FLAGS) \
    $(QUADMATH_FLAGS) 
libinterflop_fma_la_LDFLAGS = \
    $(LTO_FLAGS) -O3 $(QUADMATH_LIBS)


library_includedir = $(includedir)/
