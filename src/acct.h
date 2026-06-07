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
#ifndef __SW_ACCT_H__
#define __SW_ACCT_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "portab.h"

#include "acct_scope.h"

#define SW_ACCT_RC_OK     0
#define SW_ACCT_RC_LATER  1

#define SW_ACCT_TYPE_START   0
#define SW_ACCT_TYPE_STOP    1
#define SW_ACCT_TYPE_INTERIM 2
#define SW_ACCT_TYPE_STOP_TIME 3

typedef struct __sw_acct_entry_t {
  /* Scope-specific accting data */
  sw_acct_scope_entry_t *scope;
  uint32_t ip;
  int type;
  char *event_context;
  /* Entry data */
  time_t record_created;
  /* Control */
  uint32_t method_map;
  struct __sw_acct_entry_t *next;
} sw_acct_entry_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif
  extern int sw_acct_commit PROTO((int final_flag));
  extern int sw_acct_submit PROTO((uint32_t ip, int type, char *event_context));

#ifdef __cplusplus
}
#endif

#endif
