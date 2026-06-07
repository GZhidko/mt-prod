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
#ifndef __SW_ACCT_SCOPE_H__
#define __SW_ACCT_SCOPE_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "portab.h"

typedef struct __sw_acct_scope_entry_t {
  int id;
  void *prv;
  struct __sw_acct_scope_entry_t *next;
} sw_acct_scope_entry_t;

typedef struct __sw_acct_scope_handler_t {
  CONST char *name;
  int id;
  int (*new) PROTO((sw_acct_scope_entry_t *s, uint32_t ip,
                    int type, char *event_context));
  char *(*pretty) PROTO((sw_acct_scope_entry_t *s));
  int (*del) PROTO((sw_acct_scope_entry_t *s));
} sw_acct_scope_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_acct_scope_create();
  extern int sw_acct_scope_new PROTO((sw_acct_scope_entry_t **s,
                                      uint32_t ip, int type,
                                      char *event_context));
  extern char *sw_acct_scope_pretty PROTO((sw_acct_scope_entry_t *s));
  extern int sw_acct_scope_free PROTO((sw_acct_scope_entry_t *s));

#ifdef __cplusplus
}
#endif

#endif
