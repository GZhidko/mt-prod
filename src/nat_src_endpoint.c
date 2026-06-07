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
#include "nat_src_endpoint.h"
#include "nat_src_endpoint_ip.h"
#include "nat_src_endpoint_udp.h"
#include "nat_src_endpoint_tcp.h"
#include "nat_src_endpoint_icmp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Protocol handler modules registration */
static sw_nat_src_endpoint_handler_t *global_nat_src_endpoint_handlers[] = {
  &ip_nat_src_endpoint_handler,
  &udp_nat_src_endpoint_handler,
  &tcp_nat_src_endpoint_handler,
  &icmp_nat_src_endpoint_handler,
  NULL
};

int sw_nat_src_endpoint_create(sw_netset_t *prv_ns) {
  sw_netset_t *pub_ns;
  int idx;
  for(idx=0;global_nat_src_endpoint_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d calling new(), proto %d", __FILE__, __LINE__,
             global_nat_src_endpoint_handlers[idx]->protocol);
    if (global_nat_src_endpoint_handlers[idx]->create  &&
        global_nat_src_endpoint_handlers[idx]->create(&pub_ns, prv_ns) == -1)
      return -1;
  }
  return 0;
}

int sw_nat_src_endpoint_new(sw_netset_t *prv_ns, uint32_t prv_ip) {
  int idx;
  for(idx=0;global_nat_src_endpoint_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d calling new() for private IP %s, proto %d",
             __FILE__, __LINE__, sw_pretty_ip(prv_ip),
             global_nat_src_endpoint_handlers[idx]->protocol);
    if (global_nat_src_endpoint_handlers[idx]->new  &&
        global_nat_src_endpoint_handlers[idx]->new(prv_ns, prv_ip) == -1)
      return -1;
  }
  return 0;
}

int sw_nat_src_endpoint_del(sw_netset_t *prv_ns, uint32_t prv_ip) {
  int idx;
  for(idx=0;global_nat_src_endpoint_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d calling del() for private IP %s, proto %d",
             __FILE__, __LINE__, sw_pretty_ip(prv_ip),
             global_nat_src_endpoint_handlers[idx]->protocol);
    if (global_nat_src_endpoint_handlers[idx]->del  &&
        global_nat_src_endpoint_handlers[idx]->del(prv_ns, prv_ip) == -1)
      return -1;
  }
  return 0;
}

int sw_nat_src_endpoint_assign(sw_tuple_t *tuple_chain_a, 
                               sw_tuple_t *tuple_chain_b,
                               uint32_t prv_ip) {
  int idx;
  if (!tuple_chain_a || !tuple_chain_b)
    return 0;
  if (!tuple_chain_a || !tuple_chain_b) {
    sw_log("%s:%d mis-sized tuples", __FILE__, __LINE__);
    return -1;
  }
  if (tuple_chain_a->protocol == tuple_chain_b->protocol) {
    for(idx=0;global_nat_src_endpoint_handlers[idx];idx++) {
      if (global_nat_src_endpoint_handlers[idx]->protocol == \
          tuple_chain_a->protocol) {
        if (sw_debug_flags & SW_DEBUG_NAT_SRC)
          sw_log("%s:%d calling assign() for private IP %s, proto %d",
                 __FILE__, __LINE__, prv_ip ? sw_pretty_ip(prv_ip) : "?",
                 global_nat_src_endpoint_handlers[idx]->protocol);
        return global_nat_src_endpoint_handlers[idx]->assign(tuple_chain_a,
                                                             tuple_chain_b,
                                                             prv_ip);
      }
    }
    return sw_nat_src_endpoint_assign(tuple_chain_a->next,
                                      tuple_chain_b->next,
                                      prv_ip);
  }
  sw_log("%s:%d lame tuples %s vs %s", __FILE__, __LINE__, 
         sw_tuple_pretty(tuple_chain_a),
         sw_tuple_pretty(tuple_chain_b));
  return -1;
}
