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
# $Log: not supported by cvs2svn $
# Revision 1.3  2006/03/22 17:14:22  ilya
# typo
#
# Revision 1.2  2006/03/22 17:12:26  ilya
# demo filter added
#
# Revision 1.1  2006/03/22 17:11:54  ilya
# initial revision
#
#
EXTRA_DIST = anonymous demo example
install-data-local:
	$(NORMAL_INSTALL)
	@for ff in anonymous; do \
		if test -f $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/filters/$$ff; then \
			echo "WARNING: packet filter file $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/filters/$$ff not overwritten"; \
		else \
			$(mkinstalldirs) $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/filters; \
			$(INSTALL_DATA) @top_srcdir@/doc/filters/$$ff $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/filters; \
		fi; \
	done

uninstall-local:
	@$(NORMAL_UNINSTALL)
	$(MY_RM_PROGRAM) $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/filters/anonymous

