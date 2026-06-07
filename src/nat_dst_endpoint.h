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
#ifndef __SW_NAT_DST_ENDPOINT_H__
#define __SW_NAT_DST_ENDPOINT_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "tuple.h"
#include "portab.h"

typedef struct __sw_nat_dst_endpoint_handler_t {
  uint16_t protocol;
  int (*assign) PROTO((sw_tuple_t *new, uint32_t ip, uint16_t port));
} sw_nat_dst_endpoint_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_dst_endpoint_assign PROTO((sw_tuple_t *tuple_chain_a,
                                               uint32_t ip, uint16_t port));

#ifdef __cplusplus
}
#endif

#endif
