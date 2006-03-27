##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Mark Grondona <mgrondona@llnl.gov>
#
#  SYNOPSIS:
#    X_AC_LIBSYSFS
#
#  DESCRIPTION:
#    Adds support for the "--with-libsysfs=PATH" configure script option.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC or equivalent.
##*****************************************************************************

AC_DEFUN([X_AC_LIBSYSFS],
[
  AC_MSG_CHECKING([for libsysfs])
  savedLIBS="$LIBS"
  LIBS="-lsysfs $LIBS"
  AC_LINK_IFELSE(
     [AC_LANG_PROGRAM([[#include <sysfs/libsysfs.h>]], 
                     [[char buf[4096]; sysfs_get_mnt_path (buf, 4096)]])],
     [ac_have_libsysfs=yes], 
     [ac_have_sysfs=no]
  )
  AC_MSG_RESULT([${ac_have_libsysfs=no}]) 
  if test "$ac_have_libsysfs" = "yes"; then
    LIBSYSFS_LIBS="-lsysfs"
    AC_DEFINE([HAVE_LIBSYSFS], [1], [Define to 1 if you have libsysfs])
  else
    AC_MSG_ERROR("Unable to find working libsysfs library!")
  fi

  AC_MSG_CHECKING([for sysfs_open_device_tree in libsysfs])
  AC_LINK_IFELSE(
	  [AC_LANG_PROGRAM(
		  [[#include <sysfs/libsysfs.h>]],
		  [[struct sysfs_device *d = sysfs_open_device_tree("/sys"); 
		    sysfs_close_device(d);]])],
	  [AC_DEFINE([HAVE_SYSFS_OPEN_DEVICE_TREE], [1], 
		         [Define to 1 if libsysfs has sysfs_open_device_tree.])],
	  [ac_have_sysfs_open_device_tree=no]
  )
  AC_MSG_RESULT([${ac_have_sysfs_open_device_tree=yes}]) 

  LIBS=$saveLIBS;
  AC_SUBST([LIBSYSFS_LIBS])
])
