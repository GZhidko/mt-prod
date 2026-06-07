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
#include "pretty.h"
#include "tuple.h"
#include "frag_type.h"
#include "frag_type_ip.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Protocol handler modules registration */
static sw_frag_type_handler_t *global_frag_type_handlers[] = {
  &ip_frag_type_handler,
  NULL
};

int sw_frag_type(uint32_t *type, sw_tuple_t *tuple_chain) {
  int idx;

  if (!tuple_chain)
    return 0;

  for(idx=0;global_frag_type_handlers[idx];idx++) {
    if (global_frag_type_handlers[idx]->protocol == tuple_chain->protocol) {
      return global_frag_type_handlers[idx]->fun(type, tuple_chain);
    }
  }

  return sw_frag_type(type, tuple_chain->next);
}
