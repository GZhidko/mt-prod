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
#ifndef __SW_ACCT_METHOD_DETAIL_H__
#define __SW_ACCT_METHOD_DETAIL_H__

#include "acct_method.h"

typedef struct __sw_acct_method_detail_handler_t {
  CONST char *name;
  int id;
  int (*commit) PROTO((char *buffer, size_t length,
                       sw_acct_scope_entry_t *s, time_t delay));
} sw_acct_method_detail_handler_t;

extern sw_acct_method_handler_t acct_method_detail;
extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_acct_method_detail_create();
  extern int sw_acct_method_detail_commit PROTO((char *buffer, size_t length,
                                                 sw_acct_scope_entry_t *s,
                                                 time_t delay));

#ifdef __cplusplus
}
#endif

#endif
