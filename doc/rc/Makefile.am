#
# Sweetspot -- an OSI layer 3 captive portal.
# Written by Ilya Etingof <ilya@glas.net>, 2006.
#
# Partially based on http://chillispot.org.
# Copyright (C) 2003, 2004, 2005 Mondru AB.
# The contents of this file may be used under the terms of the GNU
# General Public License Version 2, provided that the above copyright
# notice and this permission notice is included in all copies or
# substantial portions of the software.
#
# $Log$
#
EXTRA_DIST = upstream.sh downstream.sh inner.sh outer.sh sweet_watchdog.sh

# Install sweet_watchdog.sh to /usr/local/bin
install-data-local:
	$(NORMAL_INSTALL)
	@for ff in upstream.sh downstream.sh inner.sh outer.sh; do \
		... \
	done
	$(mkinstalldirs) $(DESTDIR)/usr/local/bin
	$(INSTALL_SCRIPT) @top_srcdir@/doc/rc/sweet_watchdog.sh $(DESTDIR)/usr/local/bin/sweet_watchdog.sh
	@chmod +x $(DESTDIR)/usr/local/bin/sweet_watchdog.sh

uninstall-local:
	@$(NORMAL_UNINSTALL)
	@for ff in upstream.sh downstream.sh inner.sh outer.sh; do \
		$(MY_RM_PROGRAM) $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/rc/$$ff; \
	done
	$(MY_RM_PROGRAM) $(DESTDIR)/usr/local/bin/sweet_watchdog.sh

