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
#include <netinet/ip.h>
#include "pretty.h"
#include "filter.h"
#include "tuple.h"
#include "tuple_udp.h"
#include "filter_match_udp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int match_port(sw_filter_target_t *tg, uint16_t port) {
  int idx;
  if (tg->op == SW_FILTER_TARGET_RG) {
    if (!tg->port_lo && !tg->port_hi)
      return 1;
    if (port >= tg->port_lo && port <= tg->port_hi)
      return 1;
  } else {
    if (!tg->port_cnt)
      return 1;
    if (tg->op == SW_FILTER_TARGET_EQ)
      for (idx = 0; idx < tg->port_cnt; idx++)
        if (tg->ports[idx] == port)
          return 1;
    if (tg->op == SW_FILTER_TARGET_NE)
      for (idx = 0; idx < tg->port_cnt; idx++)
        if (tg->ports[idx] != port)
          return 1;
    if (tg->op == SW_FILTER_TARGET_LT && tg->ports[0] > port)
      return 1;
    if (tg->op == SW_FILTER_TARGET_LE && tg->ports[0] >= port)
      return 1;
    if (tg->op == SW_FILTER_TARGET_GT && tg->ports[0] < port)
      return 1;
    if (tg->op == SW_FILTER_TARGET_GE && tg->ports[0] <= port)
      return 1;
  }
  return 0;
}

static int __filter_match(sw_filter_rule_t **rule, sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b, int mode) {
  sw_tuple_udp_t *prv_a = tuple_chain_a->prv;
  sw_tuple_udp_t *prv_b = tuple_chain_b->prv;

  if (!*rule)
    return 0;

  if (!match_port(&(*rule)->src, ntohs(prv_a->port))) {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d [%d] src UDP port %s mismatch", 
             __FILE__, __LINE__, (*rule)->num, sw_pretty_port(prv_a->port));
    return 0;
  }
  if (!match_port(&(*rule)->dst, ntohs(prv_b->port))) {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d [%d] dst UDP port %s mismatch", 
             __FILE__, __LINE__, (*rule)->num, sw_pretty_port(prv_b->port));
    return 0;
  }

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d [%d] matched %s->%s",
           __FILE__, __LINE__, (*rule)->num,
           sw_pretty_port(ntohs(prv_a->port)), 
           sw_pretty_port(ntohs(prv_b->port)));

  return sw_filter_match(rule, tuple_chain_a->next, tuple_chain_b->next, mode);
}

sw_filter_match_handler_t udp_filter_match_handler = {
  IPPROTO_UDP,
  &__filter_match
};
