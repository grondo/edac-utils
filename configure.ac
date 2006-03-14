#
# This file must be processed with autoconf to generate a configure script
#

AC_INIT
X_AC_META
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/lib/edac.h])
AC_CANONICAL_HOST

#
#  Automake support
# 
AM_INIT_AUTOMAKE([$PACKAGE], [$VERSION])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
#
# Checks for programs.
#
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

X_AC_DEBUG

AC_CHECK_HEADER([sysfs/libsysfs.h], [],
		[AC_MSG_ERROR(
	         [Required header file sysfs/libsysfs.h not found.])
		])

X_AC_LIBSYSFS

AC_CONFIG_FILES([
   Makefile
   src/Makefile
   src/lib/Makefile
   src/util/Makefile
   src/etc/Makefile
 ]
)

AC_OUTPUT

