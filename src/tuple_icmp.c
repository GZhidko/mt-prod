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
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include "sweetspot.h"
#include "fnv.h"
#include "pretty.h"
#include "tuple.h"
#include "tuple_icmp.h"
#include "tuple_checksum.h"
#include "log.h"
#include "tuple_ip.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __fetch(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b,
                   char *packet, int length) {
  sw_tuple_icmp_t *prv_a, *prv_b;
  struct icmphdr *icmph = (struct icmphdr *) packet;
  struct icmp *icmp = (struct icmp *) (packet);
  struct icmphdr *icmph2 = (struct icmphdr *) (packet + 28);
  if (length < sizeof(struct icmphdr)) {
    sw_log("%s:%d short ICMP packet %d < %d",
           __FILE__, __LINE__, length, sizeof(struct icmphdr));
    return -1;
  }


#ifdef USE_MEMMGR
    sw_memgmt_tuple_buf_t * buf;
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_icmp_t)) == -1) return -1;
    prv_a = (sw_tuple_icmp_t *)&buf->buf;
    prv_a->mem_idx = buf->idx;
#else
    prv_a = (sw_tuple_icmp_t *)malloc(sizeof(sw_tuple_icmp_t));

  if (!prv_a) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
#endif
#ifdef USE_MEMMGR
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_icmp_t)) == -1) return -1;
    prv_b = (sw_tuple_icmp_t *)&buf->buf;
    prv_b->mem_idx = buf->idx;
#else
    prv_b = (sw_tuple_icmp_t *)malloc(sizeof(sw_tuple_icmp_t));

  if (!prv_b) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    free(prv_a);
    return -1;
  }
#endif
  prv_a->type = prv_b->type = icmph->type;

  tuple_chain_a->prv = prv_a;
  tuple_chain_b->prv = prv_b;

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d got ICMP type %d", __FILE__, __LINE__, prv_a->type);



  switch(prv_a->type) {
  case ICMP_ECHO:
  case ICMP_ECHOREPLY:
    prv_a->id = prv_b->id = icmph->un.echo.id;
    prv_a->seq = prv_b->seq = icmph->un.echo.sequence;
    return 0;
  case ICMP_TIMESTAMP:
  case ICMP_TIMESTAMPREPLY:
  case ICMP_REDIRECT:
      return 0; /* No higher protocols supported */

  case ICMP_TIME_EXCEEDED:
    //prv_a->type = prv_b->type = icmph2->type;
    prv_a->id = prv_b->id = icmph2->un.echo.id;
    prv_a->seq = prv_b->seq = icmph2->un.echo.sequence;
    return 0;
  }

  return sw_tuple_fetch(&(tuple_chain_a->next),
                        &(tuple_chain_b->next),
                        IPPROTO_IP,
                        packet+sizeof(struct icmphdr),
                        length-sizeof(struct icmphdr));
}


static int __commit(char *packet, int length,
                    sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b,
                    sw_tuple_t *tuple_chain_org_a,
                    sw_tuple_t *tuple_chain_org_b) {
  sw_tuple_icmp_t *prv_a = tuple_chain_a->prv, *prv_b = tuple_chain_b->prv;
  struct icmphdr *icmph = (struct icmphdr *) packet;
  struct icmphdr *icmph2 = (struct icmphdr *) (packet + 28);
  struct iphdr *iphdr =  (struct iphdr *) (packet + 8);
  sw_tuple_ip_t *prv2_a = tuple_chain_a->prev->prv, *prv2_b = tuple_chain_b->prev->prv;
  uint16_t cs;
  int rc;

  if (length < sizeof(struct icmphdr)) {
    sw_log("%s:%d short ICMP packet %d < %d",
           __FILE__, __LINE__, length, sizeof(struct icmphdr));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d ICMP type %d", __FILE__, __LINE__, prv_a->type);



  if(prv_a->type != ICMP_TIME_EXCEEDED)
  {
     if (sw_tuple_checksum_decrement(&cs, ntohs(icmph->checksum), 1,
                                      (unsigned char *)&(icmph->un),
                                      sizeof(icmph->un)) == -1) {
        return -1;
      }
  }





  switch(prv_a->type) {
  case ICMP_TIMESTAMP:
  case ICMP_TIMESTAMPREPLY:
  case ICMP_REDIRECT:

    return 0; /* No higher protocols supported */
  }

  rc = sw_tuple_commit(packet+sizeof(struct icmphdr),
                       length-sizeof(struct icmphdr),
                       tuple_chain_a->next,
                       tuple_chain_b->next,
                       tuple_chain_org_a->next,
                       tuple_chain_org_b->next);

  switch(prv_a->type) {
  case ICMP_ECHO:
  case ICMP_ECHOREPLY: /* HACK: prefer alternative value */
    icmph->un.echo.id = icmph->un.echo.id == prv_a->id ? \
      prv_b->id : prv_a->id;
    icmph->un.echo.sequence = icmph->un.echo.sequence == prv_a->seq ? \
      prv_b->seq : prv_a->seq;
    break;
  case ICMP_TIME_EXCEEDED:


      if (sw_tuple_checksum_decrement(&cs, ntohs(icmph->checksum), 1,
                                      (unsigned char *)(iphdr),
                                      sizeof(struct icmphdr)+sizeof(struct iphdr)) == -1) {
        return -1;
      }


      iphdr->saddr = prv2_b->ip;
      icmph2->un.echo.id = icmph2->un.echo.id == prv_a->id ? \
        prv_b->id : prv_a->id;
      icmph2->un.echo.sequence = icmph2->un.echo.sequence == prv_a->seq ? \
        prv_b->seq : prv_a->seq;

      if (sw_tuple_checksum_increment(&cs, cs, 1,
                                      (unsigned char *)(iphdr),
                                      sizeof(struct icmphdr)+sizeof(struct iphdr)) == -1) {
        return -1;
      }



    break;

  }



  if(prv_a->type != ICMP_TIME_EXCEEDED)
  if (sw_tuple_checksum_increment(&cs, cs, 1,
                                  (unsigned char *)&(icmph->un),
                                  sizeof(icmph->un)) == -1) {
    return -1;
  }






  icmph->checksum = htons(cs);

  return rc;
}


static uint32_t __hash(sw_tuple_t *tuple_chain) {
  sw_tuple_icmp_t *prv = tuple_chain->prv;
  struct _h {
    uint16_t protocol;
    int8_t type;
    uint16_t id;
    uint16_t seq;
  } h;
  memset(&h, 0, sizeof(h));
  h.protocol = tuple_chain->protocol;
  h.type = prv->type;
  h.id = prv->id;
  h.seq = prv->seq;
  return fnv_32a_buf((char *)&h, sizeof(h), sw_tuple_hash(tuple_chain->next));
}

static uint32_t __hash_simpl__(sw_tuple_t *tuple_chain) {
  sw_tuple_icmp_t *prv = tuple_chain->prv;
  struct _h {
    uint16_t protocol;
    int8_t type;
    uint16_t id;
    uint16_t seq;
  } h;
  memset(&h, 0, sizeof(h));
  h.protocol = tuple_chain->protocol;
  h.type = prv->type;
  return fnv_32a_buf((char *)&h, sizeof(h), sw_tuple_hash_simpl(tuple_chain->next));
}

static int __cmp(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b) {
  sw_tuple_icmp_t *prv_a = tuple_chain_a->prv;
  sw_tuple_icmp_t *prv_b = tuple_chain_b->prv;
  if (prv_a->type != prv_b->type ||
      prv_a->id != prv_b->id ||
      prv_a->seq != prv_b->seq) {
    if (sw_debug_flags & SW_DEBUG_TUPLE)
      sw_log("%s:%d ICMP type %d/%d differ %d != %d || %d != %d",
             __FILE__, __LINE__, 
             prv_a->type, prv_b->type,
             prv_a->id, prv_b->id,
             prv_a->seq, prv_b->seq);
    return 1;
  }  

  return sw_tuple_cmp(tuple_chain_a->next, tuple_chain_b->next);
}

static int __clone(sw_tuple_t *new_tuple_chain, sw_tuple_t *tuple_chain) {

#ifdef USE_MEMMGR
    sw_memgmt_tuple_buf_t * buf;
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_icmp_t)) == -1) return -1;
    new_tuple_chain->prv = (sw_tuple_icmp_t *)&buf->buf;
    sw_tuple_icmp_t * tmp = (sw_tuple_icmp_t *)new_tuple_chain->prv;
    memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_icmp_t));
    tmp->mem_idx = buf->idx;
#else

  new_tuple_chain->prv = (sw_tuple_icmp_t *)malloc(sizeof(sw_tuple_icmp_t));
  if (!new_tuple_chain->prv) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memcpy(new_tuple_chain->prv, tuple_chain->prv, sizeof(sw_tuple_icmp_t));
#endif
  return sw_tuple_clone(&new_tuple_chain->next, tuple_chain->next);
}

static char *__pretty(sw_tuple_t *tuple_chain) {
  sw_tuple_icmp_t *prv = tuple_chain->prv;
  char *b = sw_pretty_get_buffer();
  sprintf(b, " type %d id %d seq %d/%s", prv->type, ntohs(prv->id), 
          ntohs(prv->seq), sw_tuple_pretty(tuple_chain->next));
  return b;
}

static int __free(sw_tuple_t *tuple_chain) {

#ifdef USE_MEMMGR
    if (tuple_chain->prv)
    {
      sw_tuple_icmp_t * tmp = (sw_tuple_icmp_t *)tuple_chain->prv;
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

sw_tuple_handler_t icmp_tuple_handler = {
  "ICMP",
  IPPROTO_ICMP,
  &__fetch,
  &__commit,
  &__hash,
  &__cmp,
  &__clone,
  &__pretty,
  &__free,
  &__hash_simpl__
};
