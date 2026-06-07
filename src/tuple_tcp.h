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
#ifndef __SW_TUPLE_TCP_H__
#define __SW_TUPLE_TCP_H__

#include "tuple.h"
#include "portab.h"

typedef struct __sw_tuple_tcp_t {
  int mem_idx;
  uint16_t port;
  u_int8_t flags;
} sw_tuple_tcp_t;
#pragma pack(push, 1)
typedef struct __sw_tuple_tcp_pseudo_hdr_t {
  uint32_t saddr;
  uint32_t daddr;
  uint8_t reserved;
  uint8_t protocol;
  uint16_t tcp_len;
} sw_tuple_tcp_pseudo_hdr_t;
#pragma pack(pop)
extern int global_debug_flag;

extern sw_tuple_handler_t tcp_tuple_handler;

#endif
