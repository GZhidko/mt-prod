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
#ifndef __SW_NAT_TUPLE_ICMP_H__
#define __SW_NAT_TUPLE_ICMP_H__

extern int global_debug_flag;

typedef struct __sw_nat_tuple_icmp_hash_t {
  uint16_t protocol;
  uint16_t id;
  uint16_t seq;
} sw_nat_tuple_icmp_t;

extern sw_tuple_handler_t icmp_nat_tuple_handler;

#endif
