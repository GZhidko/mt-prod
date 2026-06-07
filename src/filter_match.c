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
#include "filter.h"
#include "filter_match.h"
#include "filter_match_ip.h"
#include "filter_match_udp.h"
#include "filter_match_tcp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Protocol handler modules registration */
static sw_filter_match_handler_t *global_filter_match_handlers[] = {
  &ip_filter_match_handler,
  &udp_filter_match_handler,
  &tcp_filter_match_handler,
  NULL
};

int sw_filter_match(sw_filter_rule_t **rule,
              sw_tuple_t *tuple_chain_a, 
              sw_tuple_t *tuple_chain_b, 
              int mode) {
  int idx;

  if (!tuple_chain_a && !tuple_chain_b)
    return 1; /* Once we are here, there's a match */

  if (!tuple_chain_a || !tuple_chain_b) {
    sw_log("%s:%d mis-sized tuples", __FILE__, __LINE__);
    return -1;
  }

  if (tuple_chain_a->protocol != tuple_chain_b->protocol) {
    sw_log("%s:%d lame tuples %s vs %s", __FILE__, __LINE__, 
           sw_tuple_pretty(tuple_chain_a), sw_tuple_pretty(tuple_chain_b));
    return -1;
  }

  if (*rule) {
    if ((*rule)->proto && (*rule)->proto != tuple_chain_a->protocol) {
      if (sw_debug_flags & SW_DEBUG_FILTER)
        sw_log("%s:%d [%d] protocol mismatch %s != %s", 
               __FILE__, __LINE__, (*rule)->num, 
               sw_pretty_proto((*rule)->proto),
               sw_pretty_proto(tuple_chain_a->protocol));
      return 0;
    }

    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d [%d] matched proto %s == %s", __FILE__, __LINE__, 
             (*rule)->num, sw_pretty_proto((*rule)->proto),
             sw_pretty_proto(tuple_chain_a->protocol));
  }

  for(idx=0;global_filter_match_handlers[idx];idx++) {
    if (global_filter_match_handlers[idx]->protocol ==
        tuple_chain_a->protocol) {
      if (sw_debug_flags & SW_DEBUG_FILTER)
        sw_log("%s:%d filter match proto tuple", __FILE__, __LINE__);
      return global_filter_match_handlers[idx]->fun(rule, 
                                                    tuple_chain_a, 
                                                    tuple_chain_b,
                                                    mode);
    }
  }

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d [%d] missing handler for proto %s", __FILE__, __LINE__, 
           (*rule)->num, sw_pretty_proto((*rule)->proto));
  
  /* Missing proto handlers, consider a match and move forward */
  return sw_filter_match(rule, tuple_chain_a->next, 
                         tuple_chain_b->next, mode);
}
