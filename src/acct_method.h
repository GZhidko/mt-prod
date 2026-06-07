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
#ifndef __SW_ACCT_METHOD_H__
#define __SW_ACCT_METHOD_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "portab.h"

#define SW_ACCT_METHOD_RC_OK     0
#define SW_ACCT_METHOD_RC_LATER  1

typedef struct __sw_acct_method_handler_t {
  CONST char *name; /* acct method name */
  int id; /* bitmap method ID */
  int (*create)();
  int (*destroy)();
  int (*commit)(sw_acct_entry_t *acct_entry, time_t delay);
} sw_acct_method_handler_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif
  /* Backend acct methods management */
  extern int sw_acct_method_create();
  extern int sw_acct_method_destroy();
  extern int sw_acct_method_commit PROTO((sw_acct_entry_t *e, int final_flag));

#ifdef __cplusplus
}
#endif

#endif
