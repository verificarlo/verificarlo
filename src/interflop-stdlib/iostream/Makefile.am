lib_LTLIBRARIES = libinterflop_logger.la

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

libinterflop_logger_la_SOURCES = \
    logger.c 

libinterflop_logger_la_CFLAGS = \
    $(LTO_FLAGS) -O3 \
    -fno-stack-protector \
    -D__INTERFLOP_BOOTSTRAP__ \
    -I$(top_srcdir)/.. \
    $(WARNING_FLAGS)
libinterflop_logger_la_LDFLAGS = \
    $(LTO_FLAGS) -O3

libinterflop_logger_la_includedir = $(includedir)/
