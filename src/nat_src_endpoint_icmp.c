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
#include <netinet/ip.h>
#include "lhash.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_icmp.h"
#include "nat_src_endpoint_icmp.h"
#include "lhstat.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static uint16_t global_seq_chunk;
static float global_k;

static LHASH *global_ip_hash=(LHASH*)0;

static unsigned long __hash_ip_cb(sw_nat_src_endpoint_icmp_t *e) {
  return e->prv_ip;
}

static int __cmp_ip_cb(sw_nat_src_endpoint_icmp_t *e1, 
                       sw_nat_src_endpoint_icmp_t *e2) {
  return e1->prv_ip - e2->prv_ip;
}

static int __create(sw_netset_t **pub_ns, sw_netset_t *prv_ns) {
  global_k = ceilf((float)sw_netset_size(prv_ns)/sw_netset_size(*pub_ns));
  global_seq_chunk = (int)(65535/global_k);
  if (global_seq_chunk < SW_NAT_SRC_ENDPOINT_CHUNK_SIZE_MIN) {
    sw_log("%s:%d too small public/private ICMP seq ratio: %d", __FILE__, __LINE__, global_seq_chunk);
    return -1;
  }

  if (sw_debug_flags & (SW_DEBUG_NAT_SRC|SW_DEBUG_BREATH|SW_DEBUG_RELATIONS)) 
    sw_log("%s:%d ICMP sequence chunk size %d for %d private addrs", 
           __FILE__, __LINE__, global_seq_chunk, sw_netset_size(prv_ns));

  global_ip_hash = lh_new(__hash_ip_cb, __cmp_ip_cb,0);
  if (!global_ip_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  sw_lhstat_new("snat-icmp", global_ip_hash);

  return 0;
}

static int __new(sw_netset_t *prv_ns, uint32_t prv_ip) {
  sw_nat_src_endpoint_icmp_t *e;
  uint32_t prv_num;
  spinlock  *sp = NULL;

  e = (sw_nat_src_endpoint_icmp_t *)malloc(sizeof(sw_nat_src_endpoint_icmp_t));
  if (!e) {
    sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
    return -1;
  }

  e->prv_ip = prv_ip;

  prv_num = sw_netset_idx(prv_ns, prv_ip);

  e->lseq = (int)((prv_num % (int)global_k) * global_seq_chunk);
  e->hseq =  e->lseq + global_seq_chunk - 1;
  e->seq = e->lseq;

  if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
    sw_log("%s:%d created ICMP SNAT sequence range %d-%d for private %s",
           __FILE__, __LINE__, e->lseq, e->hseq, sw_pretty_ip(prv_ip));

  lh_insert(global_ip_hash, (void *)e,&sp);
  spinlock_unlock(sp);

  return 0;
}

static int __del(sw_netset_t *prv_ns, uint32_t prv_ip) {
  sw_nat_src_endpoint_icmp_t *e, qe;
  qe.prv_ip = prv_ip;
  spinlock  *sp = NULL;

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d drop ICMP endpoints entry for private %s",
           __FILE__, __LINE__, sw_pretty_ip(prv_ip));

  e = (sw_nat_src_endpoint_icmp_t *)lh_delete(global_ip_hash, (void *)&qe,&sp);

  free(e);
  spinlock_unlock(sp);

  return 0;
}

static int __assign(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b, 
                    uint32_t prv_ip) {
  sw_tuple_icmp_t *prv_a = tuple_chain_a->prv;
  sw_nat_src_endpoint_icmp_t *e, qe;
  spinlock  *sp = NULL;



  qe.prv_ip = prv_ip;
  if(tuple_chain_a->prev->prev == NULL)
  e = (sw_nat_src_endpoint_icmp_t *)lh_retrieve(global_ip_hash, (void *)&qe,&sp);
  else
  e = (sw_nat_src_endpoint_icmp_t *)lh_retrieve(global_ip_hash, (void *)&qe,NULL);

  if (!e) {
    sw_log("%s:%d missing endpoint for private %s",
	   __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    spinlock_unlock(sp);
    return -1;
  }

  prv_a->seq = htons(e->seq);

  if (sw_debug_flags & SW_DEBUG_NAT_SRC)
    sw_log("%s:%d next public ICMP sequence %d for private %s",
           __FILE__, __LINE__, e->seq, sw_pretty_ip(prv_ip));

  if (e->seq >= e->hseq) {
    if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
      sw_log("%s:%d ICMP sequence wrapover to %d for private %s",
             __FILE__, __LINE__, e->lseq, sw_pretty_ip(prv_ip));
    e->seq = e->lseq;
  } else {
    e->seq++;
  }

  sw_tuple_t *next_a =tuple_chain_a->next;
  sw_tuple_t *next_b =tuple_chain_b->next;


  int ret = sw_nat_src_endpoint_assign(next_a,
                                    next_b,
                                    prv_ip);
   spinlock_unlock(sp);
   return ret;
}

sw_nat_src_endpoint_handler_t icmp_nat_src_endpoint_handler = {
  IPPROTO_ICMP,
  &__create,
  &__new,
  &__del,
  &__assign
};

int sw_nat_src_endpoint_icmp_getpub(uint16_t *pub_lseq, 
                                    uint16_t *pub_hseq,
                                    uint32_t prv_ip) {
  sw_nat_src_endpoint_icmp_t *e, qe;
  qe.prv_ip = prv_ip;
  spinlock  *sp = NULL;

  if (!global_ip_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }

  e = (sw_nat_src_endpoint_icmp_t *)lh_retrieve(global_ip_hash, (void *)&qe,&sp);
  if (!e) {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d no ICMP seq range yet for private IP %s",
             __FILE__, __LINE__, sw_pretty_ip(prv_ip));
    spinlock_unlock(sp);
    return -1;
  }

  if (pub_lseq) {
    *pub_lseq = e->lseq;

    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d low public ICMP seq %d for private %s",
             __FILE__, __LINE__, e->lseq, sw_pretty_ip(prv_ip));
  }
  if (pub_hseq) {
    *pub_hseq = e->hseq;

    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d high public ICMP seq %d for private %s",
             __FILE__, __LINE__, e->hseq, sw_pretty_ip(prv_ip));
  }
  spinlock_unlock(sp);
  return 0;
}
