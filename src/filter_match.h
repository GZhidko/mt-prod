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
#ifndef __SW_FILTER_MATCH_H__
#define __SW_FILTER_MATCH_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "tuple.h"
#include "filter.h"
#include "portab.h"

#define SW_FILTER_MATCH_INWARD    1
#define SW_FILTER_MATCH_OUTWARD   2

typedef struct __sw_filter_match_handler_t {
  uint8_t protocol;
  int (*fun) PROTO((sw_filter_rule_t **rule, 
                    sw_tuple_t *tuple_chain_a, 
                    sw_tuple_t *tuple_chain_b,
                    int mode));
} sw_filter_match_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_filter_match PROTO((sw_filter_rule_t **rule,
                                    sw_tuple_t *tuple_chain_a,
                                    sw_tuple_t *tuple_chain_b,
                                    int mode));

#ifdef __cplusplus
}
#endif

#endif
