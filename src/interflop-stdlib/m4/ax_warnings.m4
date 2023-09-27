# SYNOPSIS
#
#   AX_WARNINGS
#
# DESCRIPTION
#
#   Create --enable-warnings option to enable warnings flags
#

AC_DEFUN([AX_WARNINGS], 
[
AC_ARG_ENABLE(warnings, AS_HELP_STRING([--enable-warnings],[Enable warnings compilation flag]), [ENABLE_WARNINGS="yes"])
AM_CONDITIONAL([ENABLE_WARNINGS], [test "x$ENABLE_WARNINGS" = "xyes"])
if test "x$ENABLE_WARNINGS" = "xyes"; then
   AC_DEFINE([ENABLE_WARNINGS], [],  ["Enable warnings compilation flag"])
   AC_MSG_NOTICE([warnings compilation flags are set])
fi
])