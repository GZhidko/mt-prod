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
#include "pretty.h"
#include "tuple.h"
#include "nat_dst_endpoint.h"
#include "nat_dst_endpoint_ip.h"
#include "nat_dst_endpoint_tcp.h"
#include "nat_dst_endpoint_udp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Protocol handler modules registration */
static sw_nat_dst_endpoint_handler_t *global_nat_dst_endpoint_handlers[] = {
  &ip_nat_dst_endpoint_handler,
  &tcp_nat_dst_endpoint_handler,
  &udp_nat_dst_endpoint_handler,
  NULL
};

int sw_nat_dst_endpoint_assign(sw_tuple_t *tuple_chain_a,
                               uint32_t ip, uint16_t port) {
  int idx;
  if (!tuple_chain_a)
    return 0;

  for(idx=0;global_nat_dst_endpoint_handlers[idx];idx++) {
    if (global_nat_dst_endpoint_handlers[idx]->protocol == \
        tuple_chain_a->protocol) {
      if (sw_debug_flags & SW_DEBUG_NAT_DST) 
        sw_log("%s:%d DNAT endpoint proto tuple", __FILE__, __LINE__);
      return global_nat_dst_endpoint_handlers[idx]->assign(tuple_chain_a, 
                                                           ip, port);
    }
  }
  return sw_nat_dst_endpoint_assign(tuple_chain_a->next, ip, port);
}
