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
# Revision 1.44  2007/07/05 08:29:04  ilya
# terminology fix in description
#
# Revision 1.43  2007/07/02 17:16:26  ilya
# physical limits tracking and reporting moved from user session handling
# code into a dedicated 'gauge' subsystem
#
# Revision 1.42  2007/06/27 13:35:28  ilya
# updated
#
# Revision 1.41  2007/06/20 09:49:02  ilya
# split accounting methods framework off accounting records management module
#
# Revision 1.40  2007/05/29 17:05:25  ilya
# roll back all PPTP changes
# this protocol is way too kludgy to NAT ;-\
#
# Revision 1.39  2007/03/06 10:34:28  ilya
# nat_src_public.[ch] dropped
#
# Revision 1.38  2007/02/27 14:58:15  ilya
# nat_src_endpoint_pgre.[ch] added
#
# Revision 1.37  2007/02/27 08:18:52  ilya
# ttuple_pptp.c[ch] added
#
# Revision 1.36  2007/02/26 16:55:25  ilya
# pptp->pgre
#
# Revision 1.35  2007/02/26 16:41:37  ilya
# gre->pptp
#
# Revision 1.34  2007/02/26 08:10:01  ilya
# nat_src_endpoint_gre.[ch] added
#
# Revision 1.33  2007/02/22 15:58:01  ilya
# tuple_gre.[ch] added
#
# Revision 1.32  2006/12/11 19:16:44  ilya
# frag_tuple_icmp.[ch] dropped
#
# Revision 1.31  2006/12/03 23:01:03  ilya
# tuple_checksum.c/h added
#
# Revision 1.29  2006/11/30 20:19:12  ilya
# new files added
#
# Revision 1.28  2006/09/12 13:44:42  ilya
# public ICMP sequence tracket merged with public TCP/UDP ports one
#
# Revision 1.27  2006/09/12 12:15:54  ilya
# netset.[ch] added
#
# Revision 1.26  2006/08/24 18:17:48  ilya
# support selective debugging
#
# Revision 1.25  2006/08/23 21:48:36  ilya
# nat_src_range* -> nat_src_public_port*
#
# Revision 1.24  2006/08/23 21:11:10  ilya
# nat_src_range.[ch] -> nat_src_public_port.[ch]
#
# Revision 1.23  2006/08/18 09:57:33  ilya
# range.[ch] -> nat_src_range.[ch]
#
# Revision 1.22  2006/08/17 14:34:18  ilya
# dangling line
#
# Revision 1.21  2006/08/17 14:03:55  ilya
# mass files re-structure
#
# Revision 1.20  2006/08/07 14:50:08  ilya
# NAT_SRC public endpoint range resolver implemented
#
# Revision 1.19  2006/08/06 21:02:47  ilya
# nat_src.[ch] added
#
# Revision 1.17  2006/08/03 08:34:53  ilya
# sweetspot is now user interface centric. that is "inward" now universally
# means "coming from user" and vice versa.
#
# Revision 1.16  2006/06/01 15:26:38  ilya
# lhash.[ch] added
#
# Revision 1.15  2006/03/23 16:22:52  ilya
# more propaganda
#
# Revision 1.14  2006/03/22 17:18:30  ilya
# propaganda added
#
# Revision 1.13  2006/03/15 17:38:59  ilya
# packet filter grammar parser added
#
# Revision 1.12  2006/03/10 17:52:02  ilya
# filter.[ch] added
#
# Revision 1.11  2006/03/03 15:51:19  ilya
# prototol-specific checksum functions merged into checksum.[ch]
#
# Revision 1.10  2006/03/02 14:23:20  ilya
# portab.h added
#
# Revision 1.9  2006/02/16 14:41:03  ilya
# config.[ch] -> cfg.[ch]
#
# Revision 1.8  2006/02/16 14:12:41  ilya
# log.c -> [ft]log.c
#
# Revision 1.7  2006/02/15 16:21:06  ilya
# file logging implemented
#
# Revision 1.6  2006/02/02 08:38:40  ilya
# "auth" changed for "uam" in names for better clarity
#
# Revision 1.5  2006/01/27 14:55:16  ilya
# relay.c -> upstream.c & downstream.c
#
# Revision 1.4  2006/01/25 17:39:35  ilya
# acct stuff added
#
# Revision 1.3  2005/12/27 18:11:35  ilya
# libsweetauth.a & libsweetmisc.a implemented
#
# Revision 1.2  2005/12/27 17:53:57  ilya
# libsweetauth.a implemented
#
# Revision 1.1.1.1  2005/11/25 17:32:44  ilya
# initial revision
#

sbin_PROGRAMS = sweetspot sweetuam
INCLUDES = -I@top_srcdir@ -I@top_srcdir@/src
AM_CFLAGS = -g -fPIC -D_GNU_SOURCE
AM_YFLAGS = -d

noinst_LIBRARIES = libsweetuam.a libsweetmisc.a

libsweetuam_a_SOURCES = uamclt.c uammsg.c md5.c

libsweetmisc_a_SOURCES = cfg.c pretty.c debug.c

BUILT_SOURCES       = filter_parser.h

sweetspot_SOURCES = if.c session.c gauge.c nat_src.c nat_dst.c relay_inward.c \
	relay_outward.c filter.c acct.c acct_method.c acct_method_detail.c \
	acct_method_detail_snat.c acct_method_detail_session.c \
	acct_method_detail_gauge.c acct_method_detail_event.c \
	acct_scope.c acct_scope_event.c \
	acct_scope_session.c acct_scope_gauge.c acct_scope_snat.c flog.c \
	lhash.c filter_scanner.l filter_parser.y tuple.c tuple_ip.c \
	tuple_tcp.c tuple_udp.c tuple_icmp.c tuple_checksum.c time_op.c \
	gauge_index.c gauge_index_ip.c lhstat.c fnv.c shape.c memgmt.c \
	nat_src_endpoint.c nat_src_endpoint_ip.c nat_src_endpoint_tcp.c \
	nat_src_endpoint_udp.c nat_src_endpoint_icmp.c nat_dst_endpoint.c \
	nat_dst_endpoint_ip.c nat_dst_endpoint_tcp.c nat_dst_endpoint_udp.c \
	nat_tuple.c nat_tuple_ip.c nat_tuple_icmp.c frag.c frag_tuple.c \
	frag_tuple_ip.c frag_type.c frag_type_ip.c filter_match.c \
	filter_match_ip.c filter_match_udp.c filter_match_tcp.c netset.c \
        uamsrv.c sweetspot.c sh_queue.c spinlock.c cache.c mutexCirularBuffer.c
sweetspot_LDADD = libsweetuam.a libsweetmisc.a -lpthread 

sweetuam_SOURCES = tlog.c sweetuam.c
sweetuam_LDADD = libsweetuam.a libsweetmisc.a

noinst_HEADERS = if.h session.h gauge.h uamsrv.h nat_src.h nat_dst.h \
	relay_inward.h relay_outward.h sweetspot.h sweetuam.h uamsrv.h \
	uamclt.h uammsg.h md5.h cfg.h pretty.h log.h acct.h acct_method.h \
	acct_method_detail.h acct_method_detail_snat.h \
	acct_method_detail_event.h acct_method_detail_session.h \
	acct_method_detail_gauge.h acct_scope.h \
	acct_scope_event.h acct_scope_session.h acct_scope_gauge.h \
	acct_scope_snat.h lhash.h portab.h filter.h tuple.h tuple_ip.h \
	tuple_tcp.h tuple_udp.h tuple_icmp.h tuple_checksum.h time_op.h \
	gauge_index.h gauge_index_ip.h lhstat.h fnv.h shape.h memgmt.h \
	nat_src_endpoint.h nat_src_endpoint_ip.h nat_src_endpoint_tcp.h \
	nat_src_endpoint_udp.h nat_src_endpoint_icmp.h nat_dst_endpoint.h \
	nat_dst_endpoint_ip.h nat_dst_endpoint_tcp.h nat_dst_endpoint_udp.h \
	nat_tuple.h nat_tuple_ip.h nat_tuple_icmp.h frag.h frag_tuple.h \
	frag_tuple_ip.h frag_type.h frag_type_ip.h filter_match.h \
	filter_match_ip.h filter_match_udp.h filter_match_tcp.h netset.h \
        debug.h sh_queue.h spinlock.h cache.h mutexCirularBuffer.h

