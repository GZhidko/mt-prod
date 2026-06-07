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
#ifndef __SW_LHSTAT_H__
#define __SW_LHSTAT_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <time.h>
#include "lhash.h"
#include "portab.h"

#define SW_LHSTAT_INTERVAL  600

typedef struct __sw_lhstat_data_t {
  char *name;
  LHASH * lh;
  /* Copies of previous values */
  uint32_t num_expands;
  uint32_t num_contracts;
  uint32_t num_insert;
  uint32_t num_delete;
  uint32_t num_retrieve;
  uint32_t num_hash_calls;
  uint32_t num_hash_comps;
  uint32_t num_comp_calls;
  time_t last;
  struct __sw_lhstat_data_t *next;
} sw_lhstat_data_t;

extern uint32_t sw_lhstat_flags;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_lhstat_create PROTO(());
  extern int sw_lhstat_new PROTO((CONST char *name, LHASH * lh));
  extern int sw_lhstat_do PROTO((time_t now));
  extern int sw_lhstat_del PROTO((LHASH * lh));
  extern int sw_lhstat_destroy PROTO(());

#ifdef __cplusplus
}
#endif

#endif
