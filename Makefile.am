#
# Sweetspot -- an OSI layer 3 network access controller.
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
# Revision 1.9  2007/07/05 08:29:04  ilya
# terminology fix in description
#
# Revision 1.8  2006/06/14 15:02:45  ilya
# TODO added
#
# Revision 1.7  2006/04/15 21:34:47  ilya
# CHANGES added
#
# Revision 1.6  2006/03/28 14:26:16  ilya
# include perl stuff into the distribution tarball
#
# Revision 1.5  2006/03/23 16:46:53  ilya
# updated
#
# Revision 1.4  2006/03/23 16:23:47  ilya
# README.html
#
# Revision 1.3  2006/03/23 16:22:51  ilya
# more propaganda
#
# Revision 1.2  2006/03/22 17:19:27  ilya
# credits and stuff
#
# Revision 1.1.1.1  2005/11/25 17:32:44  ilya
# initial revision
#

AUTOMAKE_OPTIONS =	foreign

if DO_PYTHON
PYSUBDIRS = python
endif DO_PYTHON
if DO_PERL
PLSUBDIRS = perl
endif DO_PERL
if DO_TESTS
TSSUBDIRS = tests
endif DO_TESTS

SUBDIRS = src doc  $(PYSUBDIRS) $(TSSUBDIRS) $(PLSUBDIRS)

EXTRA_DIST = README.html LICENSE CHANGES TODO
