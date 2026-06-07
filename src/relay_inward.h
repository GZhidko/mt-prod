/*
 * Sweetspot -- an OSI layer 3 network access controller.
 * Written by Ilya Etingof <ilya@glas.net>, 2006.
 *
 * Partially based on http://chillispot.org.
 * Copyright (C) 2003, 2004, 2005 Mondru AB.
 * The contents of this file may be used under the terms of the GNU
 * General Public License Version 2, provided that the above copyright
 * notice and this permission notice is included in all copies or
 * substantial portions of the software.
 */
#ifndef __SW_RELAY_INWARD_H__
#define __SW_RELAY_INWARD_H__

#define SW_RELAY_INNER_RC  "/usr/local/etc/sweetspot/rc/inner.sh"

typedef struct __sw_relay_inward_t {
  int socket;
  int if_index;
} sw_relay_inward_t;

extern sw_socket_handler_t relay_inward;
extern int sw_relay_inward_stats_null PROTO(());
extern int sw_relay_inward_stats_print PROTO(());

#endif
