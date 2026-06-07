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
#ifndef __SW_ACCT_SCOPE_EVENT_H__
#define __SW_ACCT_SCOPE_EVENT_H__

#include "acct_scope.h"

typedef struct __sw_acct_scope_event_entry_t {
  int type;
  time_t timestamp;
  char *event_context;
} sw_acct_scope_event_entry_t;

extern sw_acct_scope_handler_t acct_scope_event;

extern int global_debug_flag;

#endif
