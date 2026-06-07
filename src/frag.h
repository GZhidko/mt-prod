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
#ifndef __SW_FRAG_H__
#define __SW_FRAG_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "tuple.h"
#include "portab.h"

#define SW_FRAG_TRACKS_MAX  16383

typedef struct __sw_frag_track_t {
  sw_tuple_t *src;
  sw_tuple_t *dst;
  uint32_t seq;
  uint8_t active;
  pthread_mutex_t mtx;
} sw_frag_track_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_frag_create PROTO(());
  extern int sw_frag_do PROTO((sw_tuple_t **hsrc, sw_tuple_t **hdst,
                               sw_tuple_t *src, sw_tuple_t *dst));

#ifdef __cplusplus
}
#endif

#endif
