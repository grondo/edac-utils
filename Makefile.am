# Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS =               -I config
CONFIG_STATUS_DEPENDENCIES =    META
EXTRA_DIST =                    AUTHORS COPYING DISCLAIMER \
                                com.github.grondo.edac-utils.metainfo.xml \
                                INSTALL NEWS README edac-utils.spec META
SUBDIRS =                       src

dist_metainfo_DATA = com.github.grondo.edac-utils.metainfo.xml
metainfodir = $(datarootdir)/metainfo

distclean-local:
	-rm -fr autm4te*.cache autoscan.* aclocal.m4
	-rm -fr $(META_NAME)-*
