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
#ifndef __SW_ACCT_SCOPE_SNAT_H__
#define __SW_ACCT_SCOPE_SNAT_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif

#include "acct_scope.h"

typedef struct __sw_acct_scope_snat_entry_t {
  uint32_t ip;
  uint16_t udp_lport;
  uint16_t udp_hport;
  uint16_t tcp_lport;
  uint16_t tcp_hport;
  uint16_t icmp_lseq;
  uint16_t icmp_hseq;
} sw_acct_scope_snat_entry_t;

extern sw_acct_scope_handler_t acct_scope_snat;

extern int global_debug_flag;

#endif
