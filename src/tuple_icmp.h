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
#ifndef __SW_TUPLE_ICMP_H__
#define __SW_TUPLE_ICMP_H__

#include "tuple.h"
#include "portab.h"

typedef struct __sw_tuple_icmp_t {
  int mem_idx;
  uint8_t type;
  uint16_t id;
  uint16_t seq;
} sw_tuple_icmp_t;

extern int global_debug_flag;

extern sw_tuple_handler_t icmp_tuple_handler;

#endif
