dnl @synopsis AX_PYTHON_MODULE(modname[, fatal])
dnl
dnl Checks for Python 2 module.
dnl
dnl If fatal is non-empty then absence of a module will trigger an
dnl error.
dnl
dnl @category InstalledPackages
dnl @author Andrew Collier <colliera@nu.ac.za>.
dnl @version 2014-09-08
dnl @license AllPermissive

AC_DEFUN([AX_PYTHON_MODULE],[
    AC_MSG_CHECKING(python module: $1)
    $PYTHON -c "import $1" 2>/dev/null
    if test $? -eq 0;
    then
        AC_MSG_RESULT(yes)
        eval AS_TR_CPP(HAVE_PYMOD_$1)=yes
    else
        AC_MSG_RESULT(no)
        eval AS_TR_CPP(HAVE_PYMOD_$1)=no
        #
        if test -n "$2"
        then
            AC_MSG_ERROR(failed to find required python module $1)
            exit 1
        fi
    fi
])
