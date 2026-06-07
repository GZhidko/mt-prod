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
#ifndef __SW_NAT_SRC_ENDPOINT_H__
#define __SW_NAT_SRC_ENDPOINT_H__

#define SW_NAT_SRC_ENDPOINT_CHUNK_SIZE_MIN 5

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "tuple.h"
#include "netset.h"
#include "portab.h"

typedef struct __sw_nat_src_endpoint_handler_t {
  uint16_t protocol;
  int (*create) PROTO((sw_netset_t **pub_ns, sw_netset_t *prv_ns));
  int (*new) PROTO((sw_netset_t *prv_ns, uint32_t prv_ip));
  int (*del) PROTO((sw_netset_t *prv_ns, uint32_t prv_ip));
  int (*assign) PROTO((sw_tuple_t *new, sw_tuple_t *old, uint32_t prv_ip));
} sw_nat_src_endpoint_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_src_endpoint_create PROTO((sw_netset_t *prv_ns));
  extern int sw_nat_src_endpoint_new PROTO((sw_netset_t *prv_ns, 
                                            uint32_t prv_ip));
  extern int sw_nat_src_endpoint_del PROTO((sw_netset_t *prv_ns, 
                                            uint32_t prv_ip));
  extern int sw_nat_src_endpoint_assign PROTO((sw_tuple_t *tuple_chain_a, 
                                               sw_tuple_t *tuple_chain_b,
                                               uint32_t prv_ip));

#ifdef __cplusplus
}
#endif

#endif
