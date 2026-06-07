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
#include "fnv.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_tcp.h"
#include "nat_src_endpoint_ip.h"
#include "nat_src_endpoint_tcp.h"
#include "lhstat.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static uint16_t global_port_chunk;
static float global_k;

static LHASH *global_ip_hash=(LHASH*)0;
static LHASH *global_id_hash=(LHASH*)0;

static unsigned long __hash_ip_cb(sw_nat_src_endpoint_tcp_t *e) {
  return e->prv_ip;
}

static int __cmp_ip_cb(sw_nat_src_endpoint_tcp_t *e1, 
                       sw_nat_src_endpoint_tcp_t *e2) {
  return e1->prv_ip - e2->prv_ip;
}

static unsigned long __hash_id_cb(sw_nat_src_endpoint_tcp_t *e) {
  struct _h {
    uint16_t id;
    uint32_t ip;
  } h;
  memset(&h, 0, sizeof(h));
  h.id = e->id;
  h.ip = e->pub_ip;
  return fnv_32a_buf((char *)&h, sizeof(h), FNV1_32A_INIT);
}

static int __cmp_id_cb(sw_nat_src_endpoint_tcp_t *e1, 
                       sw_nat_src_endpoint_tcp_t *e2) {
  return (e1->pub_ip - e2->pub_ip) | (e1->id - e2->id);
}

static int __create(sw_netset_t **pub_ns, sw_netset_t *prv_ns) {
  global_k = ceilf((float)sw_netset_size(prv_ns)/sw_netset_size(*pub_ns));
  global_port_chunk = (int)((65536-1024)/global_k);

  if (global_port_chunk < SW_NAT_SRC_ENDPOINT_CHUNK_SIZE_MIN) {
      sw_log("%s:%d too small public/private endpoints ratio: %d",
              __FILE__, __LINE__, global_port_chunk);
      return -1;
  }

  if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_BREATH)) 
    sw_log("%s:%d nets %s %d ports chunk for %d prv addrs", 
           __FILE__, __LINE__, sw_netset_pretty(*pub_ns), 
           global_port_chunk, sw_netset_size(prv_ns));


    sw_log("%s:%d nets %s %d ports chunk for %s prv addrs",
           __FILE__, __LINE__, sw_netset_pretty(*pub_ns),
           global_port_chunk, sw_netset_pretty(prv_ns));

  global_ip_hash = lh_new(__hash_ip_cb, __cmp_ip_cb,0);
  if (!global_ip_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  sw_lhstat_new("snat-tcp-ip", global_ip_hash);

  global_id_hash = lh_new(__hash_id_cb, __cmp_id_cb,0);
  if (!global_id_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    sw_lhstat_del(global_ip_hash);
    lh_free(global_ip_hash);
    return -1;
  }

  sw_lhstat_new("snat-tcp-id", global_id_hash);

  if (sw_debug_flags & (SW_DEBUG_NETSET))
  {
      for (uint32_t i = prv_ns->ip_min; i != prv_ns->ip_max; ++i)
      {
        uint32_t prv_num = sw_netset_idx(prv_ns, i);

        uint16_t lport = (int)((prv_num % (int)global_k) * global_port_chunk + 1024);
        uint16_t hport = lport + global_port_chunk - 1;

        float k = ceilf((float)sw_netset_size(prv_ns)/sw_netset_size(*pub_ns));
        int pub_ip_num = (int)(prv_num/k);
        int pub_ip = sw_netset_ip(*pub_ns, pub_ip_num);

        sw_log("%s: private ip %s translate to public ip %s , pmin:%s pmax:%s",
           "netset.c", sw_pretty_ip(i),sw_pretty_ip(pub_ip),
           sw_pretty_port(lport), sw_pretty_port(hport));
      }

  }

  return 0;
}

static int __new(sw_netset_t *prv_ns, uint32_t prv_ip) {
  sw_nat_src_endpoint_tcp_t *e;
  uint32_t prv_num;
  spinlock  *sp_ip = NULL;
  spinlock  *sp_id = NULL;

  e = (sw_nat_src_endpoint_tcp_t *)malloc(sizeof(sw_nat_src_endpoint_tcp_t));
  if (!e) {
    sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
    return -1;
  }

  e->prv_ip = prv_ip;

  prv_num = sw_netset_idx(prv_ns, prv_ip);

  if (sw_nat_src_endpoint_ip_getpub(&e->pub_ip, prv_ip) == -1)
    return -1;

  e->lport = (int)((prv_num % (int)global_k) * global_port_chunk + 1024);
  e->hport = e->lport + global_port_chunk - 1;
  e->port = e->lport;
  e->id = (e->hport - 1024) / global_port_chunk;

  e->sequential_mapping_weight = e->static_mapping_weight = e->pub_port = 0;

  if ((sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS)) || (sw_debug_flags & SW_DEBUG_NETSET))
    sw_log("%s:%d created TCP SNAT port range %s:%s-%s for private %s",
	   __FILE__, __LINE__, sw_pretty_ip(e->pub_ip),
	   sw_pretty_port(e->lport), sw_pretty_port(e->hport),
	   sw_pretty_ip(prv_ip));

  lh_insert(global_ip_hash, (void *)e,&sp_ip);
  lh_insert(global_id_hash, (void *)e,&sp_id);
  spinlock_unlock(sp_id);

  spinlock_unlock(sp_ip);

  return 0;
}

static int __del(sw_netset_t *prv_ns, uint32_t prv_ip) {
  sw_nat_src_endpoint_tcp_t *e, qe;
  qe.prv_ip = prv_ip;
  spinlock  *sp_ip = NULL;
  spinlock  *sp_id = NULL;

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d drop TCP endpoints entry for private %s",
           __FILE__, __LINE__, sw_pretty_ip(prv_ip));

  e = (sw_nat_src_endpoint_tcp_t *)lh_delete(global_ip_hash, (void *)&qe,&sp_ip);
  if (e) {
    lh_delete(global_id_hash, (void *)e,&sp_id);
    free(e);
    spinlock_unlock(sp_id);
  }
  spinlock_unlock(sp_ip);

  return 0;
}

static int __assign(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b, 
                    uint32_t prv_ip) {
  sw_tuple_tcp_t *prv_a = tuple_chain_a->prv;
  sw_tuple_tcp_t *prv_b = tuple_chain_b->prv;
  sw_nat_src_endpoint_tcp_t *e, qe;
  uint16_t pub_port;
  spinlock  *sp = NULL;

  qe.prv_ip = prv_ip;
//  if(tuple_chain_a->prev->prev == NULL)
  e = (sw_nat_src_endpoint_tcp_t *)lh_retrieve(global_ip_hash, (void *)&qe,&sp);
//  else
//  e = (sw_nat_src_endpoint_tcp_t *)lh_retrieve(global_ip_hash, (void *)&qe,NULL);


  if (!e) {
    sw_log("%s:%d missing endpoint for private %s",
	   __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    spinlock_unlock(sp);
    return -1;
  }

  pub_port = ntohs(prv_b->port);

  if (!e->pub_port)
    e->pub_port = pub_port;

#define __SW_MAX_WEIGHT  32
  if (abs(e->pub_port - pub_port) < e->hport - e->lport) {
    if (e->static_mapping_weight < __SW_MAX_WEIGHT)
      e->static_mapping_weight++;
    if (e->sequential_mapping_weight)
      e->sequential_mapping_weight--;
  } else {
    if (e->sequential_mapping_weight < __SW_MAX_WEIGHT)
      e->sequential_mapping_weight++;
    if (e->static_mapping_weight)
      e->static_mapping_weight--;
  }

  e->pub_port = pub_port;
  /* Force sequential mapping only: avoid static-mode port collisions. */
  prv_a->port = htons(e->port);
  if (e->port >= e->hport) {
    if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
      sw_log("%s:%d TCP port wrapover to %s:%s for private %s",
             __FILE__, __LINE__, sw_pretty_ip(e->pub_ip),
             sw_pretty_port(e->lport), sw_pretty_ip(prv_ip));
    e->port = e->lport;
  } else {
    e->port++;
  }


  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d public TCP port %s:%s for private %s:%s, mode %s",
           __FILE__, __LINE__, sw_pretty_ip(e->pub_ip),
           sw_pretty_port(ntohs(prv_a->port)), sw_pretty_ip(prv_ip),
           sw_pretty_port(ntohs(prv_b->port)),
           e->sequential_mapping_weight > e->static_mapping_weight ? \
           "sequentional" : "static");
  sw_tuple_t *next_a =tuple_chain_a->next;
  sw_tuple_t *next_b =tuple_chain_b->next;
  int ret = sw_nat_src_endpoint_assign(next_a,
                                    next_b,
                                    prv_ip);
   spinlock_unlock(sp);
   return ret;
}

sw_nat_src_endpoint_handler_t tcp_nat_src_endpoint_handler = {
  IPPROTO_TCP,
  &__create,
  &__new,
  &__del,
  &__assign
};

int sw_nat_src_endpoint_tcp_getpub(uint16_t *pub_lport, 
                                   uint16_t *pub_hport,
                                   uint32_t prv_ip) {
  sw_nat_src_endpoint_tcp_t *e, qe;
  qe.prv_ip = prv_ip;
  spinlock  *sp = NULL;

  if (!global_ip_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }

  e = (sw_nat_src_endpoint_tcp_t *)lh_retrieve(global_ip_hash, (void *)&qe,&sp);
  if (!e) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d no port range yet for private IP %s",
             __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    spinlock_unlock(sp);
    return -1;
  }

  if (pub_lport) {
    *pub_lport = e->lport;

    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d low public port %s for private %s",
             __FILE__, __LINE__, sw_pretty_port(e->lport),
             sw_pretty_ip(prv_ip));
  }
  if (pub_hport) {
    *pub_hport = e->hport;

    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d high public port %s for private %s",
             __FILE__, __LINE__, sw_pretty_port(e->hport),
             sw_pretty_ip(prv_ip));
  }
   spinlock_unlock(sp);
  return 0;
}

int sw_nat_src_endpoint_tcp_getprv(uint32_t *prv_ip,
                                   uint32_t pub_ip,
                                   uint16_t pub_port) {
  sw_nat_src_endpoint_tcp_t *e, qe;
  spinlock  *sp = NULL;

  if (!global_id_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }

  qe.pub_ip = pub_ip;
  qe.id = (pub_port - 1024) / global_port_chunk;

  e = (sw_nat_src_endpoint_tcp_t *)lh_retrieve(global_id_hash, (void *)&qe,&sp);
  if (!e) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d no port for public endpoint %s:%s",
             __FILE__, __LINE__, sw_pretty_ip(pub_ip), 
             sw_pretty_port(pub_port));
    spinlock_unlock(sp);
    return -1;
  }

  if (prv_ip) {
    *prv_ip = e->prv_ip;
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d private address %s for public %s",
             __FILE__, __LINE__, sw_pretty_ip(e->prv_ip),
             sw_pretty_ip(pub_ip));
  }
  spinlock_unlock(sp);
  return 0;
}
