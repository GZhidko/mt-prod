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
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <errno.h>
#include "sweetspot.h"
#include "fnv.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_ip.h"
#include "tuple_udp.h"
#include "tuple_checksum.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static sw_tuple_udp_pseudo_hdr_t pseudo_hdr;

static int __fetch(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b,
                   char *packet, int length) {
  struct udphdr *udph = (struct udphdr *) packet;
  sw_tuple_udp_t *prv_a, *prv_b;

  if (length < sizeof(struct udphdr)) {
    sw_log("%s:%d short UDP packet %d vs %d", __FILE__, __LINE__, 
           length, sizeof(struct udphdr));
    return -1;
  }

#ifdef USE_MEMMGR
    sw_memgmt_tuple_buf_t * buf;
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_udp_t)) == -1) return -1;
    prv_a = (sw_tuple_udp_t *)&buf->buf;
    prv_a->mem_idx = buf->idx;
#else
    prv_a = (sw_tuple_udp_t *)malloc(sizeof(sw_tuple_udp_t));

  if (!prv_a) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
#endif
#ifdef USE_MEMMGR
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_udp_t)) == -1) return -1;
    prv_b = (sw_tuple_udp_t *)&buf->buf;
    prv_b->mem_idx = buf->idx;
#else

  prv_b = (sw_tuple_udp_t *)malloc(sizeof(sw_tuple_udp_t));

if (!prv_b) {
  sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
  free(prv_a);
  return -1;
}
#endif


  prv_a->port = udph->source;


  prv_b->port = udph->dest;

  tuple_chain_a->prv = prv_a;
  tuple_chain_b->prv = prv_b;

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d got UDP ports %s->%s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)),
           sw_pretty_port(ntohs(prv_b->port)));
  
  return 0; /* No higher protocols supported */
}
#ifndef CHECHSUMM_FULL_CALC
static int __commit(char *packet, int length,
                    sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b,
                    sw_tuple_t *tuple_chain_org_a,
                    sw_tuple_t *tuple_chain_org_b) {
    sw_tuple_udp_pseudo_hdr_t pseudo_hdr;
  struct udphdr *udph = (struct udphdr *) packet;
  sw_tuple_udp_t *prv_a = tuple_chain_a->prv, *prv_b = tuple_chain_b->prv;
  uint16_t cs;
  int rc;

  if (length < sizeof(struct udphdr)) {
    sw_log("%s:%d short UDP packet %d vs %d", __FILE__, __LINE__,
           length, sizeof(struct udphdr));
    return -1;
  }

  /* UDP may not carry checksum, leave it alone then */
  if (udph->check) {
    if (sw_tuple_checksum_decrement(&cs, ntohs(udph->check), 1,
                                    (unsigned char *)&(udph->source),
                                    sizeof(udph->source) +
                                    sizeof(udph->dest)) == -1) {
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
  }

  rc = sw_tuple_commit(packet+sizeof(struct udphdr),
                       length-sizeof(struct udphdr),
                       tuple_chain_a->next,
                       tuple_chain_b->next,
                       tuple_chain_org_a->next,
                       tuple_chain_org_b->next);

  udph->source = prv_a->port;
  udph->dest = prv_b->port;

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d UDP ports set to %s->%s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)),
           sw_pretty_port(ntohs(prv_b->port)));

  if (udph->check) {
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
                                    (unsigned char *)&(udph->source),
                                    sizeof(udph->source) +
                                    sizeof(udph->dest)) == -1) {
      return -1;
    }

    udph->check = htons(cs);
  }

  return rc;
}
#else
static int __commit(char *packet, int length,
                    sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b,
                    sw_tuple_t *tuple_chain_org_a,
                    sw_tuple_t *tuple_chain_org_b) {
  struct udphdr *udph = (struct udphdr *) packet;
  sw_tuple_udp_t *prv_a = tuple_chain_a->prv, *prv_b = tuple_chain_b->prv;
  uint16_t cs = 0;
  int rc;

  if (length < sizeof(struct udphdr)) {
    sw_log("%s:%d short UDP packet %d vs %d", __FILE__, __LINE__, 
           length, sizeof(struct udphdr));
    return -1;
  }

  rc = sw_tuple_commit(packet+sizeof(struct udphdr), 
                       length-sizeof(struct udphdr),
                       tuple_chain_a->next,
                       tuple_chain_b->next,
                       tuple_chain_org_a->next,
                       tuple_chain_org_b->next);

  udph->source = prv_a->port;
  udph->dest = prv_b->port;


  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d UDP ports set to %s->%s", __FILE__, __LINE__,
           sw_pretty_port(ntohs(prv_a->port)),
           sw_pretty_port(ntohs(prv_b->port)));

  if (udph->check) {
    if (tuple_chain_a->prev && tuple_chain_b->prev &&
        tuple_chain_a->prev->protocol == IPPROTO_IP &&
        tuple_chain_b->prev->protocol == IPPROTO_IP) {
      sw_tuple_ip_t *prv_ip_a = tuple_chain_a->prev->prv,
        *prv_ip_b = tuple_chain_b->prev->prv;

      pseudo_hdr.saddr = prv_ip_a->ip;
      pseudo_hdr.daddr = prv_ip_b->ip;
      pseudo_hdr.reserved = 0;
      pseudo_hdr.protocol = IPPROTO_UDP;
      pseudo_hdr.udp_len = htons(length);
      udph->check = 0;


      if (sw_tuple_checksum_increment(&cs, cs, 0,
                                      (unsigned char *)&pseudo_hdr,
                                      sizeof(pseudo_hdr)) == -1) {
        return -1;
      }
    }
    //udph->check = 0;

        if (sw_tuple_checksum_increment(&cs, cs, 1,
                                        (unsigned char *)(udph),length) == -1) {
          return -1;
        }



    udph->check = htons(cs);
  }

  return rc;
}
#endif
static uint32_t __hash(sw_tuple_t *tuple_chain) {
  sw_tuple_udp_t *prv = tuple_chain->prv;
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
  sw_tuple_udp_t *prv = tuple_chain->prv;
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
  sw_tuple_udp_t *prv_a = tuple_chain_a->prv;
  sw_tuple_udp_t *prv_b = tuple_chain_b->prv;
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
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_udp_t)) == -1) return -1;
    new_tuple_chain->prv = (sw_tuple_udp_t *)&buf->buf;
    sw_tuple_udp_t * tmp = (sw_tuple_udp_t *)new_tuple_chain->prv;
    memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_udp_t));
    tmp->mem_idx = buf->idx;
#else

  new_tuple_chain->prv = (sw_tuple_udp_t *)malloc(sizeof(sw_tuple_udp_t));
  if (!new_tuple_chain->prv) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_udp_t));
#endif
  return sw_tuple_clone(&new_tuple_chain->next, tuple_chain->next);
}

static char *__pretty(sw_tuple_t *tuple_chain) {
  sw_tuple_udp_t *prv = tuple_chain->prv;
  char *b = sw_pretty_get_buffer();
  sprintf(b, "%s/%s", sw_pretty_port(ntohs(prv->port)), 
          sw_tuple_pretty(tuple_chain->next));
  return b;
}

static int __free(sw_tuple_t *tuple_chain) {
#ifdef USE_MEMMGR
    if (tuple_chain->prv)
    {
      sw_tuple_udp_t * tmp = (sw_tuple_udp_t *)tuple_chain->prv;
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

sw_tuple_handler_t udp_tuple_handler = {
  "UDP",
  IPPROTO_UDP,
  &__fetch,
  &__commit,
  &__hash,
  &__cmp,
  &__clone,
  &__pretty,
  &__free,
  &__hash_simpl__
};
