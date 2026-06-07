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
#ifndef __SW_ACCT_SCOPE_GAUGE_H__
#define __SW_ACCT_SCOPE_GAUGE_H__

#include "acct_scope.h"

typedef struct __sw_acct_scope_gauge_entry_t {
  int type;
  /* limits */
  uint64_t max_octets_in;
  uint64_t max_octets_out;
  uint64_t max_bps_in;
  uint64_t max_bps_out;
  time_t max_duration;
  time_t max_idle;
  /* currents */
  uint64_t octets_in;
  uint64_t octets_out;
  uint64_t packets_in;
  uint64_t packets_out;
  uint64_t bps_in;
  uint64_t bps_out;
  time_t duration;
  time_t idle;
} sw_acct_scope_gauge_entry_t;

extern sw_acct_scope_handler_t acct_scope_gauge;

extern int global_debug_flag;

#endif
