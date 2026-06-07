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
#include "gauge_index.h"
#include "gauge_index_ip.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Protocol handler modules registration */
static sw_gauge_index_handler_t *global_gauge_index_handlers[] = {
  &ip_gauge_index_handler,
  NULL
};

int sw_gauge_index(uint32_t *ip, sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain) {
    sw_log("%s:%d end of tuple", __FILE__, __LINE__);    
    return -1;
  }

  for(idx=0;global_gauge_index_handlers[idx];idx++) {
    if (global_gauge_index_handlers[idx]->protocol == tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_ACCT) 
        sw_log("%s:%d gauge_index proto tuple", __FILE__, __LINE__);
      return global_gauge_index_handlers[idx]->index(ip, tuple_chain);
    }
  }
  return 0;
}
