AC_PREREQ([2.72])
AC_INIT([deadbeef-plugins-fc], [0.1])
AC_CONFIG_SRCDIR([fcdec.c])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl AC_CANONICAL_HOST

AC_PROG_CXX
AC_PROG_AR
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PATH_PROG(RM,rm,rm)

AC_LANG([C])

AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([strdup])
AC_FUNC_MALLOC

PKG_CHECK_MODULES([LIBFC14AUDIODECODER], [libfc14audiodecoder >= 2.0.0], [],
    AC_MSG_ERROR([Could not find libfc14audiodecoder >= 2.0.0 API pkgconfig file.]))

AC_CHECK_HEADER(deadbeef/deadbeef.h,
    [],[AC_MSG_ERROR([deadbeef/deadbeef.h not found])])

LT_INIT([disable-static])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
