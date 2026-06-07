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
#ifndef __SW_NETSET_H__
#define __SW_NETSET_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <netinet/ip.h>
#include "portab.h"

typedef struct __sw_netset_t {
  uint32_t ip_min;
  uint32_t ip_max;
  struct __sw_netset_t *next;
} sw_netset_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_netset_create PROTO((sw_netset_t **nethead, 
                                     CONST char *cidrs));
  extern int sw_netset_destroy PROTO((sw_netset_t *nethead));
  extern char *sw_netset_pretty PROTO((sw_netset_t *nethead));
  extern uint32_t sw_netset_size PROTO((sw_netset_t *nethead));
  extern uint32_t sw_netset_idx PROTO((sw_netset_t *nethead, uint32_t ip));
  extern uint32_t sw_netset_ip PROTO((sw_netset_t *nethead, uint32_t ip_idx));
  extern uint32_t sw_check_ip(sw_netset_t *nethead, uint32_t ip);
#ifdef __cplusplus
}
#endif

#endif
