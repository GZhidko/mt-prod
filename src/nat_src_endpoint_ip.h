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
#ifndef __SW_NAT_SRC_ENDPOINT_IP_H__
#define __SW_NAT_SRC_ENDPOINT_IP_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "nat_src_endpoint.h"
#include "netset.h"
#include "portab.h"

typedef struct __sw_nat_src_endpoint_ip_t {
  uint32_t prv_ip;
  uint32_t pub_ip;
} sw_nat_src_endpoint_ip_t;

extern int global_debug_flag;

extern sw_nat_src_endpoint_handler_t ip_nat_src_endpoint_handler;

extern sw_netset_t *global_pub_ns;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_src_endpoint_ip_getpub PROTO((uint32_t *pub_ip,
                                                  uint32_t prv_ip));

#ifdef __cplusplus
}
#endif

#endif
