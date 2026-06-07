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
#include <string.h>
#include "debug.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

uint32_t sw_debug_flags = 0;

static sw_debug_flag_t global_flags_names[] = {
  { "io",        SW_DEBUG_IO },
  { "snat",      SW_DEBUG_NAT_SRC },
  { "dnat",      SW_DEBUG_NAT_DST },
  { "filter",    SW_DEBUG_FILTER },
  { "uam",       SW_DEBUG_UAM },
  { "acct",      SW_DEBUG_ACCT },
  { "session",   SW_DEBUG_SESSION },
  { "relay",     SW_DEBUG_RELAY },
  { "tuple",     SW_DEBUG_TUPLE },
  { "netset",    SW_DEBUG_NETSET },
  { "frag",      SW_DEBUG_FRAG },
  { "core",      SW_DEBUG_CORE },
  { "relations", SW_DEBUG_RELATIONS },
  { "gauge",     SW_DEBUG_GAUGE },
  { "lhstat",    SW_DEBUG_LHSTAT },
  { "breath",    SW_DEBUG_BREATH },
  { "shape",     SW_DEBUG_SHAPE},
  { "all",       SW_DEBUG_ALL },
  { NULL, 0}
};

char *sw_debug_keywords() {
  static char buf[128];
  int idx;
  
  buf[0] = '\0';

  for(idx=0;global_flags_names[idx].name; idx++) {
    sprintf(buf+strlen(buf), "%s,", global_flags_names[idx].name);
  }

  return buf;
}

int sw_debug_create(CONST char *setup_flags) {
  char buf[128], *name;
  int idx;

  if (strlen(setup_flags) >= sizeof(buf)) {
    sw_log("%s:%d enlarge you know what", __FILE__, __LINE__);
    return -1;
  }
  name = strtok(strcpy(buf, setup_flags), ",");
  while(name) {
    for(idx=0;global_flags_names[idx].name; idx++) {
      if (!strcasecmp(global_flags_names[idx].name, name)) {
        sw_debug_flags |= global_flags_names[idx].bit;
        sw_log("%s:%d enable debug %s",  __FILE__, __LINE__, 
               global_flags_names[idx].name);
        break;
      }
    }
    if (!global_flags_names[idx].name) {
      sw_log("%s:%d obscure debug keyword \"%s\"", __FILE__, __LINE__, name);
      return -1;
    }
    name = strtok(NULL, ",");
  }
  return 0;
}
