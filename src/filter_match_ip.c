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
#include "tuple_ip.h"
#include "session.h"
#include "filter_match_ip.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __filter_match(sw_filter_rule_t **rule, sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b, int mode) {
  sw_tuple_ip_t *prv_a = tuple_chain_a->prv;
  sw_tuple_ip_t *prv_b = tuple_chain_b->prv;
  sw_filter_rule_t *rs;
  int rc;
  
  if (!*rule) {
    switch(mode) {
    case SW_FILTER_MATCH_INWARD:
      if ((rc = sw_session_get_filter_id(ntohl(prv_a->ip))) == -1) {
        return -1;
      }
      if (sw_filter_get(rule, NULL, rc) == -1) {
        return -1;
      }
      break;
    case SW_FILTER_MATCH_OUTWARD:
      if ((rc = sw_session_get_filter_id(ntohl(prv_b->ip))) == -1) {
        return -1;
      }
      if (sw_filter_get(NULL, rule, rc) == -1) {
        return -1;
      }
      break;
    default:
      return -1;
    }
  }

  for(rs=*rule;rs;rs=rs->next) {
    /* Addresses */
    if (rs->src.ip && rs->src.ip != (rs->src.mask & ntohl(prv_a->ip))) {
      if (sw_debug_flags & SW_DEBUG_FILTER)
        sw_log("%s:%d [%d] src IP %s/%s != %s",
               __FILE__, __LINE__, rs->num,
               sw_pretty_ip(rs->src.ip), sw_pretty_ip(rs->src.mask),
               sw_pretty_ip(ntohl(prv_a->ip)));
      continue;
    }
    if (rs->dst.ip && rs->dst.ip != (rs->dst.mask & ntohl(prv_b->ip))) {
      if (sw_debug_flags & SW_DEBUG_FILTER)
        sw_log("%s:%d [%d] dst IP %s/%s != %s", 
               __FILE__, __LINE__, rs->num,
               sw_pretty_ip(rs->dst.ip), sw_pretty_ip(rs->dst.mask),
               sw_pretty_ip(ntohl(prv_b->ip)));
      continue;
    }

    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d [%d] matched %s->%s",
             __FILE__, __LINE__, rs->num,
             sw_pretty_ip(ntohl(prv_a->ip)),
             sw_pretty_ip(ntohl(prv_b->ip)));

    *rule = rs;

    rc = sw_filter_match(rule, tuple_chain_a->next, tuple_chain_b->next, mode);

    if (rc) {
      if (sw_debug_flags & SW_DEBUG_FILTER)
        sw_log("%s:%d [%d] matched, decision %s %s %s",
               __FILE__, __LINE__,
               rs->num,
               rs->action == SW_FILTER_ACTION_BLOCK ? "BLOCK" :
               rs->action == SW_FILTER_ACTION_DNAT ? "DNAT" : 
               rs->action == SW_FILTER_ACTION_PASS ? "PASS" : "?",
               rs->action == SW_FILTER_ACTION_DNAT &&
               mode == SW_FILTER_MATCH_INWARD ?
               sw_pretty_ip(rs->dnat.ip) : "",
               rs->action == SW_FILTER_ACTION_DNAT && 
               mode == SW_FILTER_MATCH_INWARD ? 
               sw_pretty_port(rs->dnat.port) : "");
      return rc;
    }
  }

  return 0;
}

sw_filter_match_handler_t ip_filter_match_handler = {
  IPPROTO_IP,
  &__filter_match
};
