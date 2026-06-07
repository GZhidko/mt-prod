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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "tuple.h"
#include "tuple_ip.h"
#include "gauge_index.h"
#include "gauge_index_ip.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __index(uint32_t *ip, sw_tuple_t *tuple_chain) {
  sw_tuple_ip_t *prv = tuple_chain->prv;

  *ip = ntohl(prv->ip);

  return 0;
}

sw_gauge_index_handler_t ip_gauge_index_handler = {
  IPPROTO_IP,
  &__index
};
