dnl $Id: sundials.m4,v 1.6 2005/12/21 13:14:00 raimc Exp $

dnl
dnl look for SUNDAILS Library headers in some standard set of directories
dnl
AC_DEFUN([AC_SUNDIALS_PATH],
[ AC_MSG_CHECKING([for SUNDIALS Library headers])
  for ac_dir in                 \
    /usr/local                  \
    /usr/local/sundials         \
    /usr/local/include/sundials \
    /usr/include/sundails       \
    /usr/local/share/sundials   \
    /opt/sundails               \ 
    ;                           \
  do
    if test -r "$ac_dir/include/cvodes.h"; then
      ac_SUNDIALS_path=$ac_dir
      SUNDIALS_CFLAGS="-I$ac_dir/include"
      AC_MSG_RESULT([yes])
      break
    fi
  done

  AC_MSG_CHECKING([for SUNDIALS Library])
  ac_dir=$ac_SUNDIALS_path
  for ac_extension in a so sl dylib; do
    if test -r $ac_dir/lib/libsundials_cvodes.$ac_extension; then
      SUNDIALS_LDFLAGS="-L$ac_dir/lib"
      SUNDIALS_LIBS="-lsundials_kinsol -lsundials_cvodes -lsundials_cvode -lsundials_nvecserial -lsundials_shared"
      AC_MSG_RESULT([yes])
      break
    fi
  done
])

dnl
dnl Check --with-libsundials-cvodes[=PREFIX] is specified
dnl and libsundails is installed.
dnl
AC_DEFUN([CONFIG_LIB_SUNDIALS],
[ AC_PREREQ(2.57)dnl
  AC_ARG_WITH([sundials],
  AC_HELP_STRING([--with-sundials=PREFIX],
                 [Use SUNDIALS Library]),
              [with_sundials=$withval],
              [with_sundials=yes])
  dnl set SUNDIALS related variables
  SUNDIALS_CFLAGS=
  SUNDIALS_LDFLAGS=
  SUNDIALS_LIBS=
  if test $with_sundials = yes; then
    AC_SUNDIALS_PATH
  else
    SUNDIALS_CFLAGS="-I$with_sundials/include"
    SUNDIALS_LDFLAGS="-L$with_sundials/lib"
    SUNDIALS_LIBS="-lsundials_kinsol -lsundials_cvodes -lsundials_cvode -lsundials_nvecserial -lsundials_shared"
  fi
  dnl check if SUNDIALS Library is functional
  AC_MSG_CHECKING([correct functioning of SUNDIALS])
  AC_LANG_PUSH(C)
  dnl cach values of some global variables
  sundials_save_CFLAGS="$CFLAGS"
  sundials_save_LDFLAGS="$LDFLAGS"
  sundials_save_LIBS="$LIBS"
  dnl add SUNDIALS specific stuff to global variables
  CFLAGS="$CFLAGS $SUNDIALS_CFLAGS"
  LDFLAGS="$LDFLAGS $SUNDIALS_LDFLAGS"
  LIBS="$LIBS $SUNDIALS_LIBS -lm"
  dnl can we link a mini program with cvodes?
  AC_TRY_LINK([#include <nvector_serial.h>],
    [N_Vector y; y = N_VNew_Serial(3); N_VDestroy_Serial(y);],
    [sundials_functional=yes],
    [sundials_functional=no])

  if test $sundials_functional = yes; then
    AC_MSG_RESULT([$sundials_functional])
  else
    AC_MSG_RESULT([$sundials_functional:
                   CFLAGS=$CFLAGS
                   LDFLAGS=$LDFLAGS
                   LIBS=$LIBS])
    AC_MSG_ERROR([Can not link to SUNDIALS Library])
  fi
  dnl reset global variables to cached values
  CFLAGS=$sundials_save_CFLAGS
  LDFLAGS=$sundials_save_LDFLAGS
  LIBS=$sundials_save_LIBS
  AC_LANG_POP

  AC_DEFINE([USE_SUNDIALS_SUNDIALS], 1,
            [Define to 1 to use the SUNDIALS Library])
  AC_SUBST(USE_SUNDIALS, 1)
  AC_SUBST(SUNDIALS_CFLAGS)
  AC_SUBST(SUNDIALS_LDFLAGS)
  AC_SUBST(SUNDIALS_LIBS)
])
