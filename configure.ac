AC_PREREQ([2.69])
AC_INIT([verificarlo], [0.4.1], [])
AM_SILENT_RULES([yes])
AC_CONFIG_AUX_DIR(autoconf)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AM_INIT_AUTOMAKE([subdir-objects silent-rules foreign])
AC_PROG_LIBTOOL

LT_INIT
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
# Check for specific compilers
AC_PROG_CC([gcc])
GCC_VERSION=`$CC --version | head -n1 | awk '{print $NF;}'`
AC_MSG_CHECKING([for gcc version])
AC_DEFINE_UNQUOTED([GCC_VERSION],[$GCC_VERSION],[The gcc version])
AC_MSG_RESULT([$GCC_VERSION])
AX_COMPARE_VERSION([$GCC_VERSION],[ge],['6.0.0'],[],[AC_MSG_ERROR([At least GCC version 6 is required])])
AC_DEFINE_UNQUOTED([GCC_PATH], ["$CC"], [GCC path (for compiling)])
AC_SUBST(GCC_PATH, $CC)
AX_LLVM([4.0],[11.0.1],[all])
AC_SUBST(LLVM_CPPFLAGS, $LLVM_CPPFLAGS)
AC_SUBST(LLVM_LDFLAGS, $LLVM_LDFLAGS)

AC_PATH_PROG([CLANG], [clang], "", $LLVM_BINDIR/ $PATH)
if test -z "$CLANG"; then
   AC_PATH_PROGS([CLANG], [clang-"$LLVM_VERSION_MAJOR"."$LLVM_VERSION_MINOR" clang], "")
fi
if test -z "$CLANG"; then
  AC_MSG_ERROR([Could not find clang])
fi
AC_DEFINE_UNQUOTED([CLANG_PATH], ["$CLANG"], [CLANG path])
AC_SUBST(CLANG_PATH, $CLANG)

AC_PATH_PROG([CLANGXX], [clang++], "", $LLVM_BINDIR/ $PATH)
if test -z "$CLANGXX"; then
   AC_PATH_PROGS([CLANGXX], [clang++-"$LLVM_VERSION_MAJOR"."$LLVM_VERSION_MINOR" clang++], "")
fi
if test -z "$CLANGXX"; then
  AC_MSG_ERROR([Could not find clang++])
fi
AC_DEFINE_UNQUOTED([CLANGXX_PATH], ["$CLANGXX"], [CLANGXX path])
AC_SUBST(CLANGXX_PATH, $CLANGXX)

AC_ARG_WITH([flang],
  AS_HELP_STRING([--with-flang@<:@=DIR@:>@],
    [use flang located in DIR]),
  [with_flang="$withval"],
  [with_flang=yes])

if test "x$with_flang" = "xno"; then
   AC_MSG_NOTICE([--with-flang=no was given. Disabling Fortran support.])
else
   if test "x$with_flang" = "xyes"; then
      with_flang_path="$PATH"
   else
      with_flang_path="$with_flang"
   fi
   AC_PATH_PROG([FLANG], [flang flang-"$LLVM_VERSION_MAJOR"], [], [$with_flang_path])
   if test -z "$FLANG"; then
      AC_MSG_ERROR([`flang` cannot be found in $with_flang_path])
   fi
   dnl Check that flang version matches the llvm one
   FLANG_VERSION=`$FLANG --version 2>&1 | grep -Po '\d+.\d+.\d+'`
   AC_DEFINE_UNQUOTED([FLANG_VERSION], [$FLANG_VERSION], [The flang version])
   FLANG_VERSION_MAJOR=`echo $FLANG_VERSION | cut -d'.' -f1`
   AC_DEFINE_UNQUOTED([FLANG_VERSION_MAJOR], [$FLANG_VERSION_MAJOR], [The flang major version])

   AC_MSG_CHECKING([for flang version])
   AC_MSG_RESULT([$FLANG_VERSION])
   AX_COMPARE_VERSION([$LLVM_VERSION_MAJOR],[eq],[$FLANG_VERSION_MAJOR],
      [], [AC_MSG_ERROR([flang and llvm version must match])])

   AC_DEFINE_UNQUOTED([FLANG_PATH], ["$FLANG"], [The flang path])
   AC_SUBST(FLANG_PATH, $FLANG)
fi

AC_CHECK_LIB([c], [exit], , AC_MSG_ERROR([Could not find c library]))
AC_CHECK_LIB([m], [sin], , AC_MSG_ERROR([Could not find mpfr library]))
AC_CHECK_LIB([mpfr], [mpfr_clear], , AC_MSG_ERROR([Could not find mpfr library]))

# Reset libs after ac_check_lib We do not want each and every backend or the
# libvfcinstrument frontend to be linked against libmpfr or libgfortran for
# example. -l flags should be added individually in each Makefile.am
LIBS=""

# Check for header files.
AC_CHECK_HEADERS([fcntl.h float.h inttypes.h limits.h malloc.h stdint.h stdlib.h string.h sys/time.h unistd.h utime.h mpfr.h], , AC_MSG_ERROR([Missing required headers]))

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks if __uint128_t is supported
AC_CHECK_TYPE(__uint128_t,[cc_have_uint128="yes"], [cc_have_uint128="no"])
if test "x$cc_have_uint128" = "xno"; then
   AC_MSG_ERROR([__uint128_t is not supported])
fi

# Checks if __float128 is supported
AC_CHECK_TYPE(__float128,[cc_have_float128="yes"], [cc_have_float128="no"])
if test "x$cc_have_float128" = "xno"; then
   AC_MSG_ERROR([__float128 is not supported])
fi

# Checks if _Generic keyword is supported
AC_MSG_CHECKING([if _Generic is supported by $CC])
echo "#define TEST(X) _Generic((X), int:1, long:2)" > conftest.c
doit='$CC -c conftest.c'
if AC_TRY_EVAL(doit); then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([_Generic is not supported])
fi

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC

AC_CHECK_FUNCS([atexit dup2 floor getenv gettimeofday getpagesize getpid memset mkdir pow sqrt strcasecmp strchr strdup strerror strrchr strstr strtol strtoull tzset utime])

# Check for Python 3
AM_PATH_PYTHON([3])

if test -n "$PYTHON" && test -x "$PYTHON" ; then
  AC_MSG_CHECKING([for python])
  AC_MSG_RESULT([user defined: $with_python])
else
  if test -n "$PYTHON" ; then
    AC_MSG_WARN([specified python ($PYTHON) is not usable, searching path])
  fi

  AC_PATH_PROG([PYTHON], [python python2 python27],
               [AC_MSG_RESULT([not found])
                AC_MSG_ERROR([could not find python 2.7 or higher])])
fi

AC_ARG_ENABLE(wall, AC_HELP_STRING([--enable-wall], [Enable -Wall compilation flag]), [WALL_CFLAGS="yes"])
AM_CONDITIONAL([WALL_CFLAGS], [test "x$WALL_CFLAGS" = "xyes"])
if test "x$WALL_CFLAGS" = "xyes"; then
   AC_DEFINE([WALL_CFLAGS], [],  ["Enable -Wall compilation flag"])
   AC_MSG_NOTICE([-Wall is set])
fi

# Generate config files

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/common/Makefile
                 src/libvfcinstrument/Makefile
                 src/libvfcfuncinstrument/Makefile
                 src/vfcwrapper/Makefile
                 src/backends/Makefile
                 src/backends/interflop-ieee/Makefile
                 src/backends/interflop-mca/Makefile
                 src/backends/interflop-mca-mpfr/Makefile
                 src/backends/interflop-cancellation/Makefile
                 src/backends/interflop-bitmask/Makefile
                 src/backends/interflop-vprec/Makefile
                 tests/Makefile
                 tests/paths.sh
                ])

AC_CONFIG_FILES([verificarlo.in \
		tests/test_bitmask_backend/test.sh \
		tests/test_fortran/test.sh \
		tests/test_fortran_NAS/test.sh],
		[chmod +x tests/test_bitmask_backend/test.sh \
		tests/test_fortran/test.sh \
		tests/test_fortran_NAS/test.sh])
AC_OUTPUT
