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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <sys/types.h>
#include "sweetspot.h"
#include "fnv.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_icmp.h"
#include "nat_tuple.h"
#include "nat_tuple_icmp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static uint32_t __hash(sw_tuple_t *tuple_chain) {
  sw_tuple_icmp_t *prv = tuple_chain->prv;
  struct _h {
    uint16_t protocol;
    uint16_t id;
    uint16_t seq;
  } h;
  memset(&h, 0, sizeof(h));
  h.protocol = tuple_chain->protocol;
  h.id = prv->id;
  h.seq = prv->seq;
  return fnv_32a_buf((char *)&h, sizeof(h), sw_tuple_hash(tuple_chain->next));
}

static int __cmp(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b) {
  sw_tuple_icmp_t *prv_a = tuple_chain_a->prv;
  sw_tuple_icmp_t *prv_b = tuple_chain_b->prv;
  if (prv_a->id != prv_b->id ||
      prv_a->seq != prv_b->seq) {
    if (sw_debug_flags & SW_DEBUG_TUPLE)
      sw_log("%s:%d ICMP type %d/%d differ %d != %d || %d != %d",
             __FILE__, __LINE__, 
             prv_a->type, prv_b->type,
             prv_a->id, prv_b->id,
             prv_a->seq, prv_b->seq);
    return 1;
  }  

  return sw_nat_tuple_cmp(tuple_chain_a->next, tuple_chain_b->next);
}

sw_tuple_handler_t icmp_nat_tuple_handler = {
  "NAT ICMP",
  IPPROTO_ICMP,
  NULL,
  NULL,
  &__hash,
  &__cmp,
  NULL,
  NULL,
  NULL
};
