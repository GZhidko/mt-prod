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
# Revision 1.1  2006/03/28 14:25:42  ilya
# initial revision
#
#

MODULE_VERSION = 0.0.3

EXTRA_DIST = MANIFEST setup.py examples/async-client.py examples/sync-client.py

pyexec_LTLIBRARIES = sweetuam.la
sweetuam_la_SOURCES = sweetuammodule.c
sweetuam_la_LDFLAGS = -module -avoid-version
sweetuam_la_CPPFLAGS = -I $(PYTHON_INCLUDE) -I @top_srcdir@/src -DSW_MODULE_VERSION='"$(MODULE_VERSION)"'
sweetuam_la_LIBADD = @top_srcdir@/src/debug.o @top_srcdir@/src/tlog.o @top_srcdir@/src/cfg.o @top_srcdir@/src/uamclt.o @top_srcdir@/src/uammsg.o @top_srcdir@/src/pretty.o
