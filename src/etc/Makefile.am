##*****************************************************************************
## $Id$
##*****************************************************************************
## Process this file with automake to produce Makefile.in.
##*****************************************************************************

EXTRA_DIST = \
	labels.db \
	udev.rules.in \
	edac.init.in

install-data-local:
	$(INSTALL) -m 755 -d "$(DESTDIR)$(sysconfdir)/init.d"; \
	$(INSTALL) -m 755 $(srcdir)/edac.init \
	    "$(DESTDIR)$(sysconfdir)/init.d/edac"; \
	$(INSTALL) -m 644 -D $(srcdir)/udev.rules \
	    "$(DESTDIR)/etc/udev/rules.d/60-edac-utils.rules"; \
	$(INSTALL) -m 755 -d "$(DESTDIR)$(sysconfdir)/edac/"; \
	$(INSTALL) -m 644 $(srcdir)/labels.db "$(DESTDIR)$(sysconfdir)/edac/"; 

uninstall-local:
	-rm -f "$(DESTDIR)$(sysconfdir)/init.d/edac"
	-rm -f "$(DESTDIR)$(sysconfdir)/edac/labels.db"
	-rm -f "$(DESTDIR)$(sysconfdir)/udev/rules.d/60-edac-utils.rules"
	-rmdir "$(DESTDIR)$(sysconfdir)/edac" 2>/dev/null
