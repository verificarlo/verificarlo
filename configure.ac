AC_PREREQ([2.69])
AC_INIT([verificarlo], [0.0.1], [])
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
AC_DEFINE_UNQUOTED([GCC_PATH], ["$CC"], [GCC path (for dragonegg and replay linking)])
AC_SUBST(GCC_PATH, $CC)
AX_LLVM([3.3],[all])

AC_CHECK_LIB([mca], [_mca_seed])
AC_CHECK_LIB([c], [exit])
AC_CHECK_LIB([gfortran], [exit])
AC_CHECK_LIB([m], [sin])

# Check for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h malloc.h stdint.h stdlib.h string.h sys/time.h unistd.h utime.h])

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

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC

AC_CHECK_FUNCS([atexit floor gettimeofday memset mkdir pow sqrt strchr strdup strerror strrchr strstr strtoul tzset utime])

# Check for Python 2.7

AC_ARG_WITH([python],
            [AS_HELP_STRING([--with-python], [path to python])],
            [PYTHON="$withval"])

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

AC_MSG_CHECKING([for python >= 2.7])
ac_python_version=`$PYTHON -V 2>&1 | cut -d' ' -f2`
ac_python_version_major=`echo $ac_python_version | cut -d'.' -f1`
ac_python_version_minor=`echo $ac_python_version | cut -d'.' -f2`
ac_python_version_patch=`echo $ac_python_version | cut -d'.' -f3`
if test "$ac_python_version_major" -gt "2" || \
   (test "$ac_python_version_major" -eq "2" && \
    test "$ac_python_version_minor" -ge "7") ; then
  AC_MSG_RESULT([$PYTHON ($ac_python_version)])
else
  AC_MSG_RESULT([not found])
  AC_MSG_FAILURE([found python $ac_python_version ($PYTHON); required >= 2.7])
fi

# Generate config files

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/libmcainstrument/Makefile
                 tests/Makefile])

AC_CONFIG_FILES([verificarlo], [chmod +x verificarlo])

AC_OUTPUT
