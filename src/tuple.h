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
#ifndef __SW_TUPLE_H__
#define __SW_TUPLE_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <netinet/ip.h>
#include "portab.h"
//#include "memgmt_tuple.h"

//#define USE_MEMMGR 1

#define SW_TUPLE_PROTO_DEFAULT IPPROTO_IP

typedef struct __sw_tuple_t {
  int mem_idx;
  uint16_t protocol;
  void *prv;
  struct __sw_tuple_t *prev;
  struct __sw_tuple_t *next;
} sw_tuple_t;

typedef struct __sw_tuple_handler_t {
  CONST char *name;
  uint16_t protocol;
  int (*fetch) PROTO((sw_tuple_t *tuple_chain_a,
                      sw_tuple_t *tuple_chain_b,
                      char *packet, int length));
  int (*commit) PROTO((char *packet, int length,
                       sw_tuple_t *tuple_chain_a,
                       sw_tuple_t *tuple_chain_b,
                       sw_tuple_t *tuple_chain_org_a,
                       sw_tuple_t *tuple_chain_org_b));
  uint32_t (*hash) PROTO((sw_tuple_t *tuple_chain));
  int (*cmp) PROTO((sw_tuple_t *tuple_chain_a, sw_tuple_t *tuple_chain_b));
  int (*clone) PROTO((sw_tuple_t *new_tuple_chain, sw_tuple_t *tuple_chain));
  char *(*pretty) PROTO((sw_tuple_t *tuple_chain));
  int (*del) PROTO((sw_tuple_t *tuple_chain));
  uint32_t (*hash_smp) PROTO((sw_tuple_t *tuple_chain));
} sw_tuple_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_tuple_fetch PROTO((sw_tuple_t **tuple_chain_a,
                                   sw_tuple_t **tuple_chain_b,
                                   uint16_t protocol,
                                   char *packet,
                                   int length));
  extern int sw_tuple_commit PROTO((char *packet,
                                    int length,
                                    sw_tuple_t *tuple_chain_a,
                                    sw_tuple_t *tuple_chain_b,
                                    sw_tuple_t *tuple_chain_org_a,
                                    sw_tuple_t *tuple_chain_org_b));
  extern uint32_t sw_tuple_hash PROTO((sw_tuple_t *tuple_chain));
  extern uint32_t sw_tuple_hash_simpl PROTO((sw_tuple_t *tuple_chain));
  extern int sw_tuple_cmp PROTO((sw_tuple_t *tuple_chain_a, 
                                 sw_tuple_t *tuple_chain_b));
  extern int sw_tuple_clone PROTO((sw_tuple_t **new_tuple_chain,
                                   sw_tuple_t *tuple_chain));
  extern char *sw_tuple_pretty PROTO((sw_tuple_t *tuple_chain));
  extern int sw_tuple_free PROTO((sw_tuple_t *tuple_chain));

#ifdef __cplusplus
}
#endif

#endif
