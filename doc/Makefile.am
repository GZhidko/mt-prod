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
# Revision 1.9  2006/07/14 17:19:34  ilya
# rc.sweetspot added
#
# Revision 1.8  2006/03/23 16:22:51  ilya
# more propaganda
#
# Revision 1.7  2006/03/22 17:08:48  ilya
# SUBDIRS added
#
# Revision 1.6  2006/03/22 17:07:06  ilya
# * filters targets moved to filters/
# * config files installation improved
#
# Revision 1.1.1.1  2005/11/25 17:32:44  ilya
# initial revision
#
SUBDIRS = filters rc
EXTRA_DIST = sweetuam.conf sweetspot.conf sweetauth.cgi sweetspot.cgi sweetspot80.cgi rc.sweetspot apache-partial.conf
install-data-local:
	$(NORMAL_INSTALL)
	@for ff in sweetspot.conf sweetuam.conf; do \
		if test -f $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/$$ff; then \
			echo "WARNING: config file $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/$$ff not overwritten"; \
		else \
			$(mkinstalldirs) $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR); \
			$(INSTALL_DATA) @top_srcdir@/doc/$$ff $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR); \
		fi; \
	done

uninstall-local:
	@$(NORMAL_UNINSTALL)
	$(MY_RM_PROGRAM) $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/sweetspot.conf
	$(MY_RM_PROGRAM) $(DESTDIR)$(SW_DEFAULT_CONFIG_DIR)/sweetuam.conf

