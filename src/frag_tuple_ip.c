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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include "sweetspot.h"
#include "fnv.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_ip.h"
#include "frag_tuple.h"
#include "frag_tuple_ip.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static uint32_t __hash(sw_tuple_t *tuple_chain) {
  sw_tuple_ip_t *prv = tuple_chain->prv;
  struct _h {
    uint16_t protocol;
    uint16_t id;
    uint32_t ip;
  } h;
  memset(&h, 0, sizeof(h));
  h.protocol = tuple_chain->protocol;
  h.id = prv->id;
  h.ip = prv->ip;
  return fnv_32a_buf((char *)&h, sizeof(h), FNV1_32A_INIT);
}

static int __cmp(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b) {
  sw_tuple_ip_t *prv_a = tuple_chain_a->prv;
  sw_tuple_ip_t *prv_b = tuple_chain_b->prv;
  if (prv_a->ip != prv_b->ip || prv_a->id != prv_b->id) {
    if (sw_debug_flags & SW_DEBUG_TUPLE)
      sw_log("%s:%d IPs differ %s != %s || %d != %d", __FILE__, __LINE__,
             sw_pretty_ip(ntohl(prv_a->ip)), sw_pretty_ip(ntohl(prv_b->ip)),
             prv_a->id, prv_b->id);
    return 1;
  }
  return 0;
}

static int __clone(sw_tuple_t *new_tuple_chain, sw_tuple_t *tuple_chain) {
  new_tuple_chain->prv = (sw_tuple_ip_t *)malloc(sizeof(sw_tuple_ip_t));
  if (!new_tuple_chain->prv) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_ip_t));
  return 0;
}

sw_tuple_handler_t ip_frag_tuple_handler = {
  "FRAG IP",
  IPPROTO_IP,
  NULL,
  NULL,
  &__hash,
  &__cmp,
  &__clone,
  NULL,
  NULL
};
