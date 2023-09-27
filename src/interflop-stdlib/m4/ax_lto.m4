# SYNOPSIS
#
#   AX_LTO
#
# DESCRIPTION
#
#   Create --enable-lto option to enable Link Time Optimizer
#

AC_DEFUN([AX_LTO], 
[
AC_ARG_ENABLE(lto, AS_HELP_STRING([--enable-lto],[Enable Link Time Optimizer]), [ENABLE_LTO="yes"])
AM_CONDITIONAL([ENABLE_LTO], [test "x$ENABLE_LTO" = "xyes"])
if test "x$ENABLE_LTO" = "xyes"; then
   AC_DEFINE([ENABLE_LTO], [],  ["Enable Link Time Optimizer"])
   AC_MSG_NOTICE([Link Time Optimizer is set])
fi
])