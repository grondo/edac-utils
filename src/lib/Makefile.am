##*****************************************************************************
## $Id$
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

include_HEADERS = \
	edac.h

lib_LTLIBRARIES = \
	libedac.la

man_MANS = \
	edac.3

libedac_la_LDFLAGS = \
	-export-symbols-regex "^edac_.*" \
	-version-info $(META_LT_CURRENT):$(META_LT_REVISION):$(META_LT_AGE)

libedac_la_LIBADD = \
	$(LIBSYSFS_LIBS)

libedac_la_SOURCES = \
	libedac.c \
	edac.h


