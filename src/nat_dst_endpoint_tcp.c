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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include "pretty.h"
#include "tuple.h"
#include "tuple_tcp.h"
#include "nat_dst_endpoint_tcp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __assign(sw_tuple_t *tuple_chain_a, uint32_t ip, uint16_t port) {
  sw_tuple_tcp_t *prv_a = tuple_chain_a->prv;

  if (sw_debug_flags & SW_DEBUG_NAT_DST) 
    sw_log("%s:%d port change to %s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)));

  prv_a->port = port;

  return sw_nat_dst_endpoint_assign(tuple_chain_a->next, ip, port);
}

sw_nat_dst_endpoint_handler_t tcp_nat_dst_endpoint_handler = {
  IPPROTO_TCP,
  &__assign
};
