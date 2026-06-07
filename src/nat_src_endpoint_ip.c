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
#include <math.h>
#include <netinet/in.h>
#include "lhash.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_ip.h"
#include "nat_src_endpoint_ip.h"
#include "lhstat.h"
#include "cfg.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

sw_netset_t *global_pub_ns;

static LHASH *global_ip_hash=(LHASH*)0;

static unsigned long __hash_ip_cb(sw_nat_src_endpoint_ip_t *e) {
  return e->prv_ip;
}

static int __cmp_ip_cb(sw_nat_src_endpoint_ip_t *e1, 
                       sw_nat_src_endpoint_ip_t *e2) {
  return e1->prv_ip - e2->prv_ip;
}

static int __create(sw_netset_t **pub_ns, sw_netset_t *prv_ns) {
  CONST char *snat_cidrs = sw_config_get("snat-public-networks", NULL);

  if (!snat_cidrs) {
    sw_log("%s:%d SNAT not configured", __FILE__, __LINE__);
    return -1;
  }

  if (sw_netset_create(&global_pub_ns, snat_cidrs) == -1) {
    return -1;
  }

  *pub_ns = global_pub_ns;

  if (sw_netset_size(global_pub_ns) > sw_netset_size(prv_ns)) {
    sw_log("%s:%d private/public IPs ratio < 1", __FILE__, __LINE__);
    sw_netset_destroy(global_pub_ns);
    return -1;
  }

  global_ip_hash = lh_new(__hash_ip_cb, __cmp_ip_cb,0);
  if (!global_ip_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    sw_netset_destroy(global_pub_ns);
    return -1;
  }

  sw_lhstat_new("snat-ip", global_ip_hash);

  return 0;
}

static int __new(sw_netset_t *prv_ns, uint32_t prv_ip) {
  sw_netset_t *ns;
  sw_nat_src_endpoint_ip_t *e;
  uint32_t pub_ip, pub_ip_num, prv_num;
  float k;
   spinlock  *sp = NULL;

  if (!global_ip_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }

  e = (sw_nat_src_endpoint_ip_t *)malloc(sizeof(sw_nat_src_endpoint_ip_t));
  if (!e) {
    sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
    return -1;
  }

  prv_num = sw_netset_idx(prv_ns, prv_ip);

  k = ceilf((float)sw_netset_size(prv_ns)/sw_netset_size(global_pub_ns));
  pub_ip_num = (int)(prv_num/k);
  pub_ip = sw_netset_ip(global_pub_ns, pub_ip_num);

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d public IP %s, public position %d for private %s, private position %d",
           __FILE__, __LINE__, sw_pretty_ip(pub_ip), pub_ip_num, sw_pretty_ip(prv_ip), prv_num);
 
  if (!pub_ip) {
    sw_log("%s:%d public IP failure for private %s, position %d",
           __FILE__, __LINE__, sw_pretty_ip(prv_ip), prv_num);
    return -1;
  }
  e->prv_ip = prv_ip;
  e->pub_ip = pub_ip;

  lh_insert(global_ip_hash, (void *)e,&sp);
   spinlock_unlock(sp);

  return 0;
}

static int __del(sw_netset_t *prv_ns, uint32_t prv_ip) {
  sw_nat_src_endpoint_ip_t *e, qe;
  qe.prv_ip = prv_ip;
   spinlock  *sp = NULL;

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d drop IP endpoints entry for private %s",
           __FILE__, __LINE__, sw_pretty_ip(prv_ip));

  e = (sw_nat_src_endpoint_ip_t *)lh_delete(global_ip_hash, (void *)&qe,&sp);

  free(e);
   spinlock_unlock(sp);

  return 0;
}

static int __assign(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b,
                    uint32_t prv_ip) {
  sw_tuple_ip_t *prv_a = tuple_chain_a->prv;
  sw_tuple_ip_t *prv_b = tuple_chain_b->prv;
  sw_nat_src_endpoint_ip_t *e, qe;
  spinlock  *sp = NULL;

  prv_ip = ntohl(prv_b->ip);

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d recovering private IP %s",
           __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    
  qe.prv_ip = prv_ip;
  
  if(tuple_chain_a->prev == NULL)
  e = (sw_nat_src_endpoint_ip_t *)lh_retrieve(global_ip_hash, (void *)&qe,&sp);
  else
  e = (sw_nat_src_endpoint_ip_t *)lh_retrieve(global_ip_hash, (void *)&qe,NULL);

  if (!e) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d missing endpoint for private %s",
             __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    spinlock_unlock(sp);
    return -1;
  }

  prv_a->ip = htonl(e->pub_ip);

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d public address %s for private %s",
           __FILE__, __LINE__, sw_pretty_ip(e->pub_ip), sw_pretty_ip(prv_ip));
  sw_tuple_t *next_a =tuple_chain_a->next;
  sw_tuple_t *next_b =tuple_chain_b->next;
  int ret = sw_nat_src_endpoint_assign(next_a,
                                    next_b,
                                    prv_ip);
   spinlock_unlock(sp);
   return ret;
}

sw_nat_src_endpoint_handler_t ip_nat_src_endpoint_handler = {
  IPPROTO_IP,
  &__create,
  &__new,
  &__del,
  &__assign
};

int sw_nat_src_endpoint_ip_getpub(uint32_t *pub_ip, uint32_t prv_ip) {
  sw_nat_src_endpoint_ip_t *e, qe;
  qe.prv_ip = prv_ip;
  spinlock  *sp = NULL;

  if (!global_ip_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }

  e = (sw_nat_src_endpoint_ip_t *)lh_retrieve(global_ip_hash, (void *)&qe,&sp);
  if (!e) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d no public IP yet for private IP %s",
             __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    spinlock_unlock(sp);
    return -1;
  }

  if (pub_ip) {
    *pub_ip = e->pub_ip;

    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d public IP %s for private IP %s", __FILE__, __LINE__,
             sw_pretty_ip(e->pub_ip), sw_pretty_ip(prv_ip));
  }
  spinlock_unlock(sp);
  return 0;
}
