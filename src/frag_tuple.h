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
#ifndef __SW_FRAG_TUPLE_H__
#define __SW_FRAG_TUPLE_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <netinet/ip.h>
#include "tuple.h"
#include "portab.h"

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern uint32_t sw_frag_tuple_hash PROTO((sw_tuple_t *tuple_chain));
  extern int sw_frag_tuple_cmp PROTO((sw_tuple_t *tuple_chain_a, 
                                      sw_tuple_t *tuple_chain_b));
  extern int sw_frag_tuple_clone PROTO((sw_tuple_t **new_tuple_chain,
					sw_tuple_t *tuple_chain));

#ifdef __cplusplus
}
#endif

#endif
