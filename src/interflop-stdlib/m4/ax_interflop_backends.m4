# SYNOPSIS
#
#   AX_INTERFLOP_BACKENDS
#
# DESCRIPTION
#
# Include the interflop-backends for the configure.ac file.
#
# List of the backends:
#  - interflop-backend-bitmask
#  - interflop-backend-cancellation 
#  - interflop-backend-ieee 
#  - interflop-backend-mcaint 
#  - interflop-backend-mcaquad 
#  - interflop-backend-vprec
#  - interflop-backend-verrou
#  - interflop-backend-checkfloatmax
#  - interflop-backend-checkdenormal
#  - interflop-backend-checkcancellation

AC_DEFUN([AX_INTERFLOP_BACKENDS], [
        INTERFLOP_BACKENDS=""

    # Verificarlo backends
    if test -d "$srcdir/src/backends/interflop-backend-bitmask"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-bitmask])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-bitmask"

    fi
    if test -d "$srcdir/src/backends/interflop-backend-cancellation"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-cancellation])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-cancellation"

    fi
    if test -d "$srcdir/src/backends/interflop-backend-ieee"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-ieee])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-ieee"
    fi
    if test -d "$srcdir/src/backends/interflop-backend-mcaint"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-mcaint])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-mcaint"
    fi
    if test -d "$srcdir/src/backends/interflop-backend-mcaquad"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-mcaquad])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-mcaquad"
    fi
    if test -d "$srcdir/src/backends/interflop-backend-vprec"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-vprec])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-vprec"
    fi
    # Verrou backends
    if test -d "$srcdir/src/backends/interflop-backend-verrou"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-verrou])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-verrou"
    fi
    if test -d "$srcdir/src/backends/interflop-backend-checkfloatmax"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-checkfloatmax])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-checkfloatmax"
    fi
    if test -d "$srcdir/src/backends/interflop-backend-checkdenormal"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-checkdenormal])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-checkdenormal"
    fi
    if test -d "$srcdir/src/backends/interflop-backend-checkcancellation"; then
        AC_CONFIG_SUBDIRS([src/backends/interflop-backend-checkcancellation])
        INTERFLOP_BACKENDS="$INTERFLOP_BACKENDS interflop-backend-checkcancellation"
    fi    
    AC_DEFINE_UNQUOTED([INTERFLOP_BACKENDS], ["$INTERFLOP_BACKENDS"], [List of the interflop backends])
    AC_SUBST(INTERFLOP_BACKENDS, $INTERFLOP_BACKENDS)
])