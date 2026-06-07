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
#ifndef __SW_NAT_DST_H__
#define __SW_NAT_DST_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "tuple.h"
#include "portab.h"

#define SW_NAT_DST_RELATIONS_MAX  3746112

typedef struct __sw_nat_dst_relation_t {
  sw_tuple_t *src;
  sw_tuple_t *dst;
  sw_tuple_t *nat;
  uint32_t seq;

} sw_nat_dst_relation_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_dst_create PROTO(());
  extern int sw_nat_dst_do PROTO((sw_tuple_t *src, sw_tuple_t *dst, 
                                  sw_tuple_t *nat));
  extern int sw_nat_dst_undo PROTO((sw_tuple_t **src, sw_tuple_t *nat, 
                                    sw_tuple_t *dst));

#ifdef __cplusplus
}
#endif

#endif
