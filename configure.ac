##*****************************************************************************
## $Id$
##*****************************************************************************
## Process this file with autoconf to produce a configure script.
##*****************************************************************************
## Copyright (C) 2002-2006 The Regents of the University of California.
## Produced at Lawrence Livermore National Laboratory.
## Written by Mark Grondona <mgrondona@llnl.gov>.
## UCRL-CODE-230739.
##
## This file is part of edac-utils.
##
## This is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
## for more details.
##
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.,
## 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
##*****************************************************************************

AC_INIT
X_AC_META
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/lib/edac.h])
AC_CANONICAL_HOST

#
#  Automake support
# 
AM_INIT_AUTOMAKE([$META_NAME], [$META_VERSION])
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
   src/lib/edac.3
   src/util/Makefile
   src/util/edac-util.1
   src/util/edac-ctl.8
   src/util/edac-ctl
   src/etc/Makefile
   src/etc/edac.init
 ]
)

AC_OUTPUT

