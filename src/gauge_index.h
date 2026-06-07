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
#ifndef __SW_GAUGE_INDEX_H__
#define __SW_GAUGE_INDEX_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "tuple.h"
#include "portab.h"

#define SW_GAUGE_INDEX_INWARD    1
#define SW_GAUGE_INDEX_OUTWARD   2

typedef struct __sw_gauge_index_handler_t {
  uint8_t protocol;
  int (*index) PROTO((uint32_t *ip, sw_tuple_t *tuple_chain));
} sw_gauge_index_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_gauge_index PROTO((uint32_t *ip, sw_tuple_t *tuple_chain));

#ifdef __cplusplus
}
#endif

#endif
