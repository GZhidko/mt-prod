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
#ifndef __SW_DEBUG_H__
#define __SW_DEBUG_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "portab.h"

#define SW_DEBUG_IO           0x00000001
#define SW_DEBUG_NAT_SRC      0x00000002
#define SW_DEBUG_NAT_DST      0x00000004
#define SW_DEBUG_FILTER       0x00000008
#define SW_DEBUG_UAM          0x00000010
#define SW_DEBUG_ACCT         0x00000020
#define SW_DEBUG_SESSION      0x00000040
#define SW_DEBUG_RELAY        0x00000080
#define SW_DEBUG_TUPLE        0x00000100
#define SW_DEBUG_NETSET       0x00000200
#define SW_DEBUG_FRAG         0x00000400
#define SW_DEBUG_CORE         0x00000800
#define SW_DEBUG_RELATIONS    0x00001000  /* NAT relations */
#define SW_DEBUG_GAUGE        0x00002000  
#define SW_DEBUG_LHSTAT       0x00004000
#define SW_DEBUG_BREATH       0x00008000  /* routine logging */
#define SW_DEBUG_SHAPE        0x00010000
#define SW_DEBUG_ALL          0xffffffff

typedef struct __sw_debug_flag_t {
  char *name;
  uint32_t bit;
} sw_debug_flag_t;

extern uint32_t sw_debug_flags;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_debug_create PROTO((CONST char *setup_flags));
  extern char *sw_debug_keywords PROTO(());

#ifdef __cplusplus
}
#endif

#endif
