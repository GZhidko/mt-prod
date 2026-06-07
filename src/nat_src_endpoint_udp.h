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
#ifndef __SW_NAT_SRC_ENDPOINT_UDP_H__
#define __SW_NAT_SRC_ENDPOINT_UDP_H__

#include "nat_src_endpoint.h"
#include "portab.h"

extern int global_debug_flag;

extern sw_nat_src_endpoint_handler_t udp_nat_src_endpoint_handler;

typedef struct __sw_nat_src_endpoint_udp_t {
  /* Index */
  uint32_t pub_ip;
  uint32_t prv_ip;
  uint16_t lport;
  uint16_t hport;
  uint16_t port;
  uint16_t id;
} sw_nat_src_endpoint_udp_t;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_src_endpoint_udp_getpub PROTO((uint16_t *pub_lport,
                                                   uint16_t *pub_hport,
                                                   uint32_t prv_ip));
  extern int sw_nat_src_endpoint_udp_getprv PROTO((uint32_t *prv_ip,
                                                   uint32_t pub_ip,
                                                   uint16_t pub_port));

#ifdef __cplusplus
}
#endif

#endif
