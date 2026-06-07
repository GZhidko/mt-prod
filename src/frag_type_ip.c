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
#include "pretty.h"
#include "tuple.h"
#include "tuple_ip.h"
#include "frag_type.h"
#include "frag_type_ip.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __type(uint32_t *type, sw_tuple_t *tuple_chain) {
  sw_tuple_ip_t *prv = tuple_chain->prv;
  uint16_t frag_off = ntohs(prv->frag_off);

  if (frag_off & IP_MF) {
    if (frag_off & IP_OFFMASK) {
      *type = SW_FRAG_TYPE_BODY;
    } else {
      *type = SW_FRAG_TYPE_HEAD;
    }
  } else {
    if (frag_off & IP_OFFMASK) {
      *type = SW_FRAG_TYPE_TAIL;
    } else {
      *type = SW_FRAG_TYPE_NONE;
    }
  }

  if (sw_debug_flags & SW_DEBUG_FRAG)
    sw_log("%s:%d %s (0x%x)%s", __FILE__, __LINE__,
           *type == SW_FRAG_TYPE_NONE ? "no fragment" : "fragment", frag_off,
           *type == SW_FRAG_TYPE_HEAD ? " head" : \
           *type == SW_FRAG_TYPE_BODY ? " body" : \
           *type == SW_FRAG_TYPE_TAIL ? " tail" : "");

  return 0;
}

sw_frag_type_handler_t ip_frag_type_handler = {
  IPPROTO_IP,
  &__type
};
