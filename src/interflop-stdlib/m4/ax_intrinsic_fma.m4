# SYNOPSIS
#
#   AX_INTRINSIC_FMA
#
# DESCRIPTION
#
#   Create --enable-intrinsic-fma option to enable Fused-Multiply-Add
#

AC_DEFUN([AX_INTRINSIC_FMA], 
[
# --enable-intrinsic-fma
# Taken from configure.ac Verrou
AC_CACHE_CHECK([intrinsic fma], interflop_cv_intrinsic_fma,
  [AC_ARG_ENABLE(intrinsic-fma,
    AS_HELP_STRING([--enable-intrinsic-fma], [enables interflop-stdlib to use intrinsic fma]),
    [interflop_cv_intrinsic_fma=$enableval],
    [interflop_cv_intrinsic_fma=yes])])

if test "$interflop_cv_intrinsic_fma" != no; then
  # Check for fmaintrin.h
  AC_LANG_PUSH(C++)
  CXXFLAGS="$safe_CXXFLAGS -mfma"
  AC_MSG_CHECKING([for fmaintrin.h ])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      #include <immintrin.h>
      #include <stdlib.h>
]], [[
       double a,b,c,d;
      __m128d ai,bi,ci,di;
      ai = _mm_load_sd(&a);
      bi = _mm_load_sd(&b);
      ci = _mm_load_sd(&c);
      di = _mm_fmadd_sd(ai,bi,ci);
      d  = _mm_cvtsd_f64(di);
      return EXIT_SUCCESS;
    ]])],
    [
      AC_MSG_RESULT([yes])
      interflop_cv_intrinsic_fma=yes
    ],[
      AC_MSG_RESULT([no])
      AC_MSG_NOTICE([--enable-intrinsic-fma=no was given. Use soft-fma])
      interflop_cv_intrinsic_fma=no
  ])
  AC_LANG_POP(C++)
else
   AC_MSG_NOTICE([--enable-intrinsic-fma=no was given. Use soft-fma])
   interflop_cv_intrinsic_fma=no
fi

AM_CONDITIONAL([HAVE_FMA_INTRINSIC], [test "x$interflop_cv_intrinsic_fma" == "xyes"],[])
AC_SUBST(interflop_cv_intrinsic_fma)
])