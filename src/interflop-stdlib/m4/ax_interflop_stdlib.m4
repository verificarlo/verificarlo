# SYNOPSIS
#
#   AX_INTERFLOP_STDLIB()
#
# DESCRIPTION
#
#   Create --with-interflop-stdlib option to link a backend with 
#   the interflop-stdlib library
#

AC_DEFUN([AX_INTERFLOP_STDLIB], 
[
AC_ARG_WITH([interflop-stdlib],
  AS_HELP_STRING([--with-interflop-stdlib@<:@=DIR@:>@],
    [use interflop-stdlib located in DIR]),
  [with_interflop_stdlib="$withval"],
  [with_interflop_stdlib=yes])

if test "x$with_interflop_stdlib" = "xno"; then
   AC_MSG_ERROR([Could not find interflop-stdlib library])
elif test "x$with_interflop_stdlib" = "xyes"; then
   with_interflop_stdlib_path="$PATH"
else
   with_interflop_stdlib_path="$with_interflop_stdlib"
fi

AC_PATH_PROG([INTERFLOP_CONFIG], [interflop-config], [], [$with_interflop_stdlib_path])
if test -z "$INTERFLOP_CONFIG"; then
   AC_MSG_ERROR(
   [interflop-stdlib is required but program `interflop-config` cannot be found in $with_interflop_stdlib_path])
fi

INTERFLOP_VERSION=$($INTERFLOP_CONFIG --version)
AC_DEFINE_UNQUOTED([INTERFLOP_VERSION], ["$INTERFLOP_VERSION"], [The interflop version])
AC_SUBST(INTERFLOP_VERSION, $INTERFLOP_VERSION)

INTERFLOP_MAJOR_VERSION=$(echo $INTERFLOP_VERSION | cut -d'.' -f1)
AC_DEFINE_UNQUOTED([INTERFLOP_MAJOR_VERSION], ["$INTERFLOP_MAJOR_VERSION"], [The interflop major version])
AC_SUBST(INTERFLOP_MAJOR_VERSION, $INTERFLOP_MAJOR_VERSION)

INTERFLOP_MINOR_VERSION=$(echo $INTERFLOP_VERSION | cut -d'.' -f2)
AC_DEFINE_UNQUOTED([INTERFLOP_MINOR_VERSION], ["$INTERFLOP_MINOR_VERSION"], [The interflop minor version])
AC_SUBST(INTERFLOP_MINOR_VERSION, $INTERFLOP_MINOR_VERSION)

INTERFLOP_PATCH_VERSION=$(echo $INTERFLOP_VERSION | cut -d'.' -f3 )
AC_DEFINE_UNQUOTED([INTERFLOP_PATCH_VERSION], ["$INTERFLOP_PATCH_VERSION"], [The interflop patch version])
AC_SUBST(INTERFLOP_PATCH_VERSION, $INTERFLOP_PATCH_VERSION)

INTERFLOP_PREFIX=$($INTERFLOP_CONFIG --prefix)
AC_DEFINE_UNQUOTED([INTERFLOP_PREFIX], ["$INTERFLOP_PREFIX"], [The interflop prefix dir])
AC_SUBST(INTERFLOP_PREFIX, $INTERFLOP_PREFIX)

INTERFLOP_BINDIR=$($INTERFLOP_CONFIG --bindir)
AC_DEFINE_UNQUOTED([INTERFLOP_BINDIR], ["$INTERFLOP_BINDIR"], [The interflop bin dir])
AC_SUBST(INTERFLOP_BINDIR, $INTERFLOP_BINDIR)
  
INTERFLOP_LIBDIR=$($INTERFLOP_CONFIG --libdir)
AC_DEFINE_UNQUOTED([INTERFLOP_LIBDIR], ["$INTERFLOP_LIBDIR"], [The interflop lib dir])
AC_SUBST(INTERFLOP_LIBDIR, $INTERFLOP_LIBDIR)

INTERFLOP_INCLUDEDIR=$($INTERFLOP_CONFIG --includedir)
AC_DEFINE_UNQUOTED([INTERFLOP_INCLUDEDIR], ["$INTERFLOP_INCLUDEDIR"], [The interflop include dir])
AC_SUBST(INTERFLOP_INCLUDEDIR, $INTERFLOP_INCLUDEDIR)

INTERFLOP_DATAROOTDIR=$($INTERFLOP_CONFIG --datarootdir)
AC_DEFINE_UNQUOTED([INTERFLOP_DATAROOTDIR], ["$INTERFLOP_DATAROOTDIR"], [The interflop datarootdir dir])
AC_SUBST(INTERFLOP_DATAROOTDIR, $INTERFLOP_DATAROOTDIR)

])

