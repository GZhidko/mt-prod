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
#ifndef __SW_NAT_SRC_ENDPOINT_ICMP_H_
#define __SW_NAT_SRC_ENDPOINT_ICMP_H__

#include "nat_src_endpoint.h"
#include "portab.h"

extern int global_debug_flag;

extern sw_nat_src_endpoint_handler_t icmp_nat_src_endpoint_handler;

typedef struct __sw_nat_src_endpoint_icmp_t {
  uint32_t prv_ip;
  uint16_t lseq;
  uint16_t hseq;
  uint16_t seq;
} sw_nat_src_endpoint_icmp_t;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_src_endpoint_icmp_getpub PROTO((uint16_t *pub_lseq,
                                                    uint16_t *pub_hseq,
                                                    uint32_t prv_ip));

#ifdef __cplusplus
}
#endif

#endif
