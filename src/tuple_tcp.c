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
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include "sweetspot.h"
#include "fnv.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_ip.h"
#include "tuple_tcp.h"
#include "tuple_checksum.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static sw_tuple_tcp_pseudo_hdr_t pseudo_hdr;

static int __fetch(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b,
                   char *packet, int length) {
  struct tcphdr *tcph = (struct tcphdr *) packet;
  sw_tuple_tcp_t *prv_a, *prv_b;

  if (length < sizeof(struct tcphdr) || length < 20) {
    sw_log("%s:%d short TCP packet %d < %d",
           __FILE__, __LINE__, length, sizeof(struct tcphdr));
    return -1;
  }

#ifdef USE_MEMMGR
    sw_memgmt_tuple_buf_t * buf;
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_tcp_t)) == -1) return -1;
    prv_a = (sw_tuple_tcp_t *)&buf->buf;
    prv_a->mem_idx = buf->idx;
#else
    prv_a = (sw_tuple_tcp_t *)malloc(sizeof(sw_tuple_tcp_t));

  if (!prv_a) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
#endif
#ifdef USE_MEMMGR
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_tcp_t)) == -1) return -1;
    prv_b = (sw_tuple_tcp_t *)&buf->buf;
    prv_b->mem_idx = buf->idx;
#else

  prv_b = (sw_tuple_tcp_t *)malloc(sizeof(sw_tuple_tcp_t));

if (!prv_b) {
  sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
  free(prv_a);
  return -1;
}
#endif



  prv_a->port = tcph->source;

  prv_a->flags = 0;
  prv_a->flags = tcph->urg; 
  prv_a->flags <<= 1; prv_a->flags |= tcph->ack;
  prv_a->flags <<= 1; prv_a->flags |= tcph->psh;
  prv_a->flags <<= 1; prv_a->flags |= tcph->rst;
  prv_a->flags <<= 1; prv_a->flags |= tcph->syn;
  prv_a->flags <<= 1; prv_a->flags |= tcph->fin;


  prv_b->port = tcph->dest;

  prv_b->flags = 0;
  prv_b->flags = tcph->urg; 
  prv_b->flags <<= 1; prv_b->flags |= tcph->ack;
  prv_b->flags <<= 1; prv_b->flags |= tcph->psh;
  prv_b->flags <<= 1; prv_b->flags |= tcph->rst;
  prv_b->flags <<= 1; prv_b->flags |= tcph->syn;
  prv_b->flags <<= 1; prv_b->flags |= tcph->fin;

  tuple_chain_a->prv = prv_a;
  tuple_chain_b->prv = prv_b;

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d got TCP ports %s->%s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)),
           sw_pretty_port(ntohs(prv_b->port)));
   
  return 0; /* No higher protocols supported XXX */
}
#ifndef CHECHSUMM_FULL_CALC
static int __commit(char *packet, int length,
                    sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b,
                    sw_tuple_t *tuple_chain_org_a,
                    sw_tuple_t *tuple_chain_org_b) {
  sw_tuple_tcp_pseudo_hdr_t pseudo_hdr;
  struct tcphdr *tcph = (struct tcphdr *) packet;
  sw_tuple_tcp_t *prv_a = tuple_chain_a->prv, *prv_b = tuple_chain_b->prv;
  uint16_t cs;
  int rc;

  if (length < sizeof(struct tcphdr) || tcph->doff*4 > length) {
    sw_log("%s:%d short TCP packet %d < %d",
           __FILE__, __LINE__, length, tcph->doff*4 );
    return -1;
  }



  if (sw_tuple_checksum_decrement(&cs, ntohs(tcph->check), 1,
                                  (unsigned char *)&(tcph->source),
                                  sizeof(tcph->source) +
                                  sizeof(tcph->dest)) == -1) {
    return -1;
  }

  if (tuple_chain_org_a->prev && tuple_chain_org_b->prev &&
      tuple_chain_org_a->prev->protocol == IPPROTO_IP &&
      tuple_chain_org_b->prev->protocol == IPPROTO_IP) {
    sw_tuple_ip_t *prv_ip_a = tuple_chain_org_a->prev->prv,
      *prv_ip_b = tuple_chain_org_b->prev->prv;

    pseudo_hdr.saddr = prv_ip_a->ip;
    pseudo_hdr.daddr = prv_ip_b->ip;

    if (sw_tuple_checksum_decrement(&cs, cs, 0,
                                    (unsigned char *)&pseudo_hdr,
                                    sizeof(pseudo_hdr)) == -1) {
      return -1;
    }
  }

  rc = sw_tuple_commit(packet+tcph->doff*4,
                       length-tcph->doff*4,
                       tuple_chain_a->next,
                       tuple_chain_b->next,
                       tuple_chain_org_a->next,
                       tuple_chain_org_b->next);

  tcph->source = prv_a->port;
  tcph->dest = prv_b->port;

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d TCP ports set to %s->%s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)),
           sw_pretty_port(ntohs(prv_b->port)));

  if (tuple_chain_a->prev && tuple_chain_b->prev &&
      tuple_chain_a->prev->protocol == IPPROTO_IP &&
      tuple_chain_b->prev->protocol == IPPROTO_IP) {
    sw_tuple_ip_t *prv_ip_a = tuple_chain_a->prev->prv,
      *prv_ip_b = tuple_chain_b->prev->prv;

    pseudo_hdr.saddr = prv_ip_a->ip;
    pseudo_hdr.daddr = prv_ip_b->ip;

    if (sw_tuple_checksum_increment(&cs, cs, 0,
                                    (unsigned char *)&pseudo_hdr,
                                    sizeof(pseudo_hdr)) == -1) {
      return -1;
    }
  }

  if (sw_tuple_checksum_increment(&cs, cs, 1,
                                  (unsigned char *)&(tcph->source),
                                  sizeof(tcph->source) +
                                  sizeof(tcph->dest)) == -1) {
    return -1;
  }

  tcph->check = htons(cs);

  return rc;
}
#else
static int __commit(char *packet, int length,
                    sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b,
                    sw_tuple_t *tuple_chain_org_a,
                    sw_tuple_t *tuple_chain_org_b) {
  struct tcphdr *tcph = (struct tcphdr *) packet;
  sw_tuple_tcp_t *prv_a = tuple_chain_a->prv, *prv_b = tuple_chain_b->prv;
  uint16_t cs = 0;
  int rc;

  if (length < sizeof(struct tcphdr) || length < 20) {
    sw_log("%s:%d short TCP packet %d < %d",
           __FILE__, __LINE__, length, sizeof(struct tcphdr));
    return -1;
  }

  rc = sw_tuple_commit(packet+tcph->doff*4,
                       length-tcph->doff*4,
                       tuple_chain_a->next,
                       tuple_chain_b->next,
                       tuple_chain_org_a->next,
                       tuple_chain_org_b->next);

  tcph->source = prv_a->port;
  tcph->dest = prv_b->port;
  tcph->check = 0;


  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d TCP ports set to %s->%s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)),
           sw_pretty_port(ntohs(prv_b->port)));

  if (tuple_chain_a->prev && tuple_chain_b->prev &&
      tuple_chain_a->prev->protocol == IPPROTO_IP &&
      tuple_chain_b->prev->protocol == IPPROTO_IP) {
    sw_tuple_ip_t *prv_ip_a = tuple_chain_a->prev->prv,
      *prv_ip_b = tuple_chain_b->prev->prv;
    memset(&pseudo_hdr, 0, sizeof(sw_tuple_tcp_pseudo_hdr_t));
    pseudo_hdr.saddr = prv_ip_a->ip;
    pseudo_hdr.daddr = prv_ip_b->ip;


    pseudo_hdr.protocol = IPPROTO_TCP;
    pseudo_hdr.tcp_len = htons(length);
    if (sw_tuple_checksum_increment(&cs, cs, 0,
                                    (unsigned char *)&pseudo_hdr,
                                    sizeof(pseudo_hdr)) == -1) {
      return -1;
    }
  }

  if (sw_tuple_checksum_increment(&cs, cs, 1,
                                  (unsigned char *)(tcph),
                                  length) == -1) {
    return -1;
  }

  tcph->check = htons(cs);

  return rc;
}
#endif

static uint32_t __hash(sw_tuple_t *tuple_chain) {
  sw_tuple_tcp_t *prv = tuple_chain->prv;
  struct _h {
    uint16_t protocol;
    uint16_t port;
  } h;
  memset(&h, 0, sizeof(h));
  h.protocol = tuple_chain->protocol;
  h.port = prv->port;
  return fnv_32a_buf((char *)&h, sizeof(h), sw_tuple_hash(tuple_chain->next));
}


static uint32_t __hash_simpl__(sw_tuple_t *tuple_chain) {
  sw_tuple_tcp_t *prv = tuple_chain->prv;
  struct _h {
    uint16_t protocol;
    uint16_t port;
  } h;
  memset(&h, 0, sizeof(h));
  h.protocol = tuple_chain->protocol;
  h.port = prv->port;
  return fnv_32a_buf((char *)&h, sizeof(h), sw_tuple_hash_simpl(tuple_chain->next));
}

static int __cmp(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b) {
  sw_tuple_tcp_t *prv_a = tuple_chain_a->prv;
  sw_tuple_tcp_t *prv_b = tuple_chain_b->prv;
  if (prv_a->port != prv_b->port) {
    if (sw_debug_flags & SW_DEBUG_TUPLE)
      sw_log("%s:%d ports differ %s != %s", __FILE__, __LINE__,
             sw_pretty_port(ntohs(prv_a->port)),
             sw_pretty_port(ntohs(prv_b->port)));
    return 1;
  }  
  return sw_tuple_cmp(tuple_chain_a->next, tuple_chain_b->next);
}

static int __clone(sw_tuple_t *new_tuple_chain, sw_tuple_t *tuple_chain) {
#ifdef USE_MEMMGR
    sw_memgmt_tuple_buf_t * buf;
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_tcp_t)) == -1) return -1;
    new_tuple_chain->prv = (sw_tuple_tcp_t *)&buf->buf;
    sw_tuple_tcp_t * tmp = (sw_tuple_tcp_t *)new_tuple_chain->prv;
    memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_tcp_t));
    tmp->mem_idx = buf->idx;
#else

  new_tuple_chain->prv = (sw_tuple_tcp_t *)malloc(sizeof(sw_tuple_tcp_t));
  if (!new_tuple_chain->prv) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_tcp_t));
#endif

  return sw_tuple_clone(&new_tuple_chain->next, tuple_chain->next);
}

static char *__pretty(sw_tuple_t *tuple_chain) {
  sw_tuple_tcp_t *prv = tuple_chain->prv;
  char *b = sw_pretty_get_buffer();
  sprintf(b, "%s/%s", sw_pretty_port(ntohs(prv->port)), 
          sw_tuple_pretty(tuple_chain->next));
  return b;
}

static int __free(sw_tuple_t *tuple_chain) {
#ifdef USE_MEMMGR
    if (tuple_chain->prv)
    {
      sw_tuple_tcp_t * tmp = (sw_tuple_tcp_t *)tuple_chain->prv;
      if(tmp->mem_idx != 0)
      sw_memgmt_tuple_free_buf(tmp->mem_idx);
    }
#else
    if (tuple_chain->prv){
      free(tuple_chain->prv);
  }
#endif
  return sw_tuple_free(tuple_chain->next);
}

sw_tuple_handler_t tcp_tuple_handler = {
  "TCP",
  IPPROTO_TCP,
  &__fetch,
  &__commit,
  &__hash,
  &__cmp,
  &__clone,
  &__pretty,
  &__free,
  &__hash_simpl__
};
