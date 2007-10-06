dnl $Id: fpic.m4,v 1.2 2007/10/06 23:37:59 raimc Exp $
dnl
dnl checks if compiler can use flage -fpic for position-independent code
dnl (PIC) for use in a shared library if so variable FPIC is set to value
dnl -fpic
dnl

AC_DEFUN([AC_PROG_CC_FPIC],
[ac_test_CFLAGS=${CFLAGS+set}
ac_save_CFLAGS=$CFLAGS
CFLAGS="-fpic"
AC_CACHE_CHECK(whether $CC accepts -fpic, ac_cv_prog_cc_fpic,
       [_AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [ac_cv_prog_cc_fpic=yes],
						[ac_cv_prog_cc_fpic=no])])
CFLAGS="$ac_save_CFLAGS"
dnl TODO: -fPIC for Mac
if test $ac_cv_prog_cc_fpic = yes; then
  FPIC="-fpic"
fi[]dnl
]
)

