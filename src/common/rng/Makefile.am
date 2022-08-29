noinst_LTLIBRARIES = libvfc_rng.la

libvfc_rng_la_CFLAGS = -fPIC -static -O3
if WALL_CFLAGS
libvfc_rng_la_CFLAGS += -Wall -Wextra
endif
libvfc_rng_la_SOURCES = \
    splitmix64.c \
    xoroshiro128.c \
    vfc_rng.c
