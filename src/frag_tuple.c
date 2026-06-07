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
#include "fnv.h"
#include "frag_tuple.h"
#include "frag_tuple_ip.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Protocol handler modules registration */
static sw_tuple_handler_t *global_tuple_handlers[] = {
  &ip_frag_tuple_handler,
  NULL
};

uint32_t sw_frag_tuple_hash(sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain)
    return FNV1_32A_INIT; /* hash function seed */

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == \
        tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE) 
        sw_log("%s:%d hash proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      return global_tuple_handlers[idx]->hash(tuple_chain);
    }
  }
  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__,
         sw_pretty_proto(tuple_chain->protocol));
  return -1;
}

int sw_frag_tuple_cmp(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b) {
  int idx;

  if (tuple_chain_a && tuple_chain_b) {
    if (tuple_chain_a->protocol != tuple_chain_b->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d protocol tuples differ %s vs %s",
               __FILE__, __LINE__, 
               sw_pretty_proto(tuple_chain_a->protocol),
               sw_pretty_proto(tuple_chain_b->protocol));
      return tuple_chain_a->protocol - tuple_chain_b->protocol;
    }
  } else {
    if (tuple_chain_a) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d short protocol tuple chain B", __FILE__, __LINE__);
      return 1;
    }
    if (tuple_chain_b) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d short protocol tuple chain A", __FILE__, __LINE__);
      return -1;
    }
    if (sw_debug_flags & SW_DEBUG_TUPLE)
      sw_log("%s:%d end of protocol tuple chain", __FILE__, __LINE__);
    return 0;
  }

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain_a->protocol){
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d cmp proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      return global_tuple_handlers[idx]->cmp(tuple_chain_a,tuple_chain_b);
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__, 
         sw_pretty_proto(tuple_chain_a->protocol));
  return -1;
}

int sw_frag_tuple_clone(sw_tuple_t **new_tuple_chain,
                        sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain)
    return 0;

  *new_tuple_chain = malloc(sizeof(sw_tuple_t));
  if (!(*new_tuple_chain)) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memcpy(*new_tuple_chain, tuple_chain, sizeof(sw_tuple_t));
  (*new_tuple_chain)->next = NULL;

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE) 
        sw_log("%s:%d clone proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      return global_tuple_handlers[idx]->clone(*new_tuple_chain, tuple_chain);
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__, 
         sw_pretty_proto(tuple_chain->protocol));
  return -1;
}
