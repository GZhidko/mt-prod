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
#ifndef __SW_TUPLE_IP_H__
#define __SW_TUPLE_IP_H__

#include "tuple.h"
#include "portab.h"

typedef struct __sw_tuple_ip_t {
  int mem_idx;
  uint32_t ip;
  uint16_t id;
  uint16_t frag_off;
} sw_tuple_ip_t;

extern int global_debug_flag;

extern sw_tuple_handler_t ip_tuple_handler;

#endif
