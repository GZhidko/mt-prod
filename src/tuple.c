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
#include "tuple.h"
#include "tuple_ip.h"
#include "tuple_tcp.h"
#include "tuple_udp.h"
#include "tuple_icmp.h"
#include "log.h"
//#include "memgmt_tuple.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif



/* Protocol handler modules registration */
static sw_tuple_handler_t *global_tuple_handlers[] = {
  &ip_tuple_handler,
  &tcp_tuple_handler,
  &udp_tuple_handler,
  &icmp_tuple_handler,
  NULL
};



int sw_tuple_fetch(sw_tuple_t **tuple_chain_a,
                   sw_tuple_t **tuple_chain_b,
                   uint16_t protocol,
                   char *packet, int length) {
  int idx, rc;

  *tuple_chain_a = *tuple_chain_b = NULL;

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == protocol) {
  #ifdef USE_MEMMGR
      sw_memgmt_tuple_buf_t * buf;
      if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_t)) == -1) return -1;
      *tuple_chain_a = (sw_tuple_t*)&buf->buf;
      memset(*tuple_chain_a, 0, sizeof(sw_tuple_t));
      (*tuple_chain_a)->mem_idx = buf->idx;
  #else
      *tuple_chain_a = malloc(sizeof(sw_tuple_t));

      if (!(*tuple_chain_a)) {
        sw_log("%s:%d malloc() failed: %s",
               __FILE__, __LINE__, strerror(errno));
        return -1;
      }
      memset(*tuple_chain_a, 0, sizeof(sw_tuple_t));
  #endif

#ifdef USE_MEMMGR
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_t)) == -1) return -1;
    *tuple_chain_b = (sw_tuple_t*)&buf->buf;
    memset(*tuple_chain_b, 0, sizeof(sw_tuple_t));
    (*tuple_chain_b)->mem_idx = buf->idx;
#else
    *tuple_chain_b = malloc(sizeof(sw_tuple_t));

      if (!(*tuple_chain_b)) {
        sw_log("%s:%d malloc() failed: %s",
               __FILE__, __LINE__, strerror(errno));
        free(*tuple_chain_a);
        return -1;
      }
      memset(*tuple_chain_b, 0, sizeof(sw_tuple_t));
#endif


      (*tuple_chain_a)->protocol = (*tuple_chain_b)->protocol = protocol;

      if (sw_debug_flags & SW_DEBUG_TUPLE) 
        sw_log("%s:%d fetch proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);

      rc = global_tuple_handlers[idx]->fetch(*tuple_chain_a,
                                             *tuple_chain_b,
                                             packet, length);
      if (*tuple_chain_a && (*tuple_chain_a)->next) {
        (*tuple_chain_a)->next->prev = *tuple_chain_a;
      }
      if (*tuple_chain_b && (*tuple_chain_b)->next) {
        (*tuple_chain_b)->next->prev = *tuple_chain_b;
      }

      return rc;
    }
  }

  if (sw_debug_flags & SW_DEBUG_TUPLE)
    sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__,
           sw_pretty_proto(protocol));

  return -1;
}

int sw_tuple_commit(char *packet, int length,
                    sw_tuple_t *tuple_chain_a,
                    sw_tuple_t *tuple_chain_b,
                    sw_tuple_t *tuple_chain_org_a,
                    sw_tuple_t *tuple_chain_org_b) {
  int idx;

  if(!tuple_chain_a || !tuple_chain_b)
    return 0;

  if (!tuple_chain_org_a) tuple_chain_org_a = tuple_chain_a;
  if (!tuple_chain_org_b) tuple_chain_org_b = tuple_chain_b;

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain_a->protocol &&
        tuple_chain_a->protocol == tuple_chain_b->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE) 
        sw_log("%s:%d commit proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      return global_tuple_handlers[idx]->commit(packet, length,
                                                tuple_chain_a,
                                                tuple_chain_b,
                                                tuple_chain_org_a,
                                                tuple_chain_org_b);
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__, 
         sw_pretty_proto(tuple_chain_a->protocol));
  return -1;
}

uint32_t sw_tuple_hash(sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain)
    return FNV1_32A_INIT; /* hash function seed */

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d hasher %s tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name, 
               sw_tuple_pretty(tuple_chain));
      return global_tuple_handlers[idx]->hash(tuple_chain);
    }
  }
  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__,
         sw_pretty_proto(tuple_chain->protocol));
  return -1;
}

uint32_t sw_tuple_hash_simpl(sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain)
    return FNV1_32A_INIT; /* hash function seed */

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d hasher %s tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name,
               sw_tuple_pretty(tuple_chain));
      return global_tuple_handlers[idx]->hash_smp(tuple_chain);
    }
  }
  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__,
         sw_pretty_proto(tuple_chain->protocol));
  return -1;
}

int sw_tuple_cmp(sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b) {
  int idx;

  if(tuple_chain_a && tuple_chain_b) {
    if (tuple_chain_a->protocol != tuple_chain_b->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d protocol tuples differ %s vs %s", __FILE__, __LINE__, 
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
    if (global_tuple_handlers[idx]->protocol == tuple_chain_a->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d cmp proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      return global_tuple_handlers[idx]->cmp(tuple_chain_a, tuple_chain_b);
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__, 
         sw_pretty_proto(tuple_chain_a->protocol));
  return -1;
}

int sw_tuple_clone(sw_tuple_t **new_tuple_chain,sw_tuple_t *tuple_chain) {
  int idx, rc;

  if (!tuple_chain)
    return 0;

#ifdef USE_MEMMGR
    sw_memgmt_tuple_buf_t * buf;
    if(sw_memgmt_tuple_get_buf(&buf,sizeof(sw_tuple_t)) == -1) return -1;
    *new_tuple_chain = (sw_tuple_t*)&buf->buf;
    memcpy(*new_tuple_chain, tuple_chain, sizeof(sw_tuple_t));
    (*new_tuple_chain)->mem_idx = buf->idx;
#else
    *new_tuple_chain = malloc(sizeof(sw_tuple_t));
  if (!(*new_tuple_chain)) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
    memcpy(*new_tuple_chain, tuple_chain, sizeof(sw_tuple_t));
#endif



  (*new_tuple_chain)->next = (*new_tuple_chain)->prev = NULL;

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE) 
        sw_log("%s:%d clone proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      rc = global_tuple_handlers[idx]->clone(*new_tuple_chain, tuple_chain);
      if (*new_tuple_chain && (*new_tuple_chain)->next) {
        (*new_tuple_chain)->next->prev = *new_tuple_chain;
      }
      return rc;
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__, 
         sw_pretty_proto(tuple_chain->protocol));
  return -1;
}

char *sw_tuple_pretty(sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain)
    return "";

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain->protocol) {
      char *b = sw_pretty_get_buffer();
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d make pretty proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      sprintf(b, "%s:%s", sw_pretty_proto(tuple_chain->protocol),
              global_tuple_handlers[idx]->pretty(tuple_chain));
      return b;
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__,
         sw_pretty_proto(tuple_chain->protocol));
  return "?";
}

int sw_tuple_free(sw_tuple_t *tuple_chain) {
  int idx, rc;

  if (!tuple_chain)
    return 0;

  for(idx=0;global_tuple_handlers[idx];idx++) {
    if (global_tuple_handlers[idx]->protocol == tuple_chain->protocol) {
      if (sw_debug_flags & SW_DEBUG_TUPLE)
        sw_log("%s:%d free proto tuple %s", __FILE__, __LINE__,
               global_tuple_handlers[idx]->name);
      rc = global_tuple_handlers[idx]->del(tuple_chain);
      //free(tuple_chain);
#ifdef USE_MEMMGR
     sw_memgmt_tuple_free_buf(tuple_chain->mem_idx);
#else
     free(tuple_chain);
#endif

      return rc;
    }
  }

  sw_log("%s:%d unsupported protocol %s", __FILE__, __LINE__,
         sw_pretty_proto(tuple_chain->protocol));
  return -1;
}
