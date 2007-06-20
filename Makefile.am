# Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS =               -I config
CONFIG_STATUS_DEPENDENCIES =    META
EXTRA_DIST =                    AUTHORS COPYING ChangeLog DISCLAIMER \
                                INSTALL NEWS README edac-utils.spec META
SUBDIRS =                       src

distclean-local:
	-rm -fr autm4te*.cache autoscan.* aclocal.m4
	-rm -fr $(META_NAME)-*
