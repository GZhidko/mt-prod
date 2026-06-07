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
#include <arpa/inet.h>
#include <netdb.h>
#include "filter.h"
#include "pretty.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static char bufs[SW_PRETTY_BUFFERS_MAX][SW_PRETTY_BUFFERS_SIZE];
static int idx = 0;

char *sw_pretty_get_buffer() {
  if (idx > SW_PRETTY_BUFFERS_MAX-2)
    idx = 0;
  else
    idx++;
  bufs[idx][0] = '\0';
  return bufs[idx];
}

char *sw_pretty_ip(uint32_t ip) {
  struct in_addr addr;
  addr.s_addr = htonl(ip);
  return strcpy(sw_pretty_get_buffer(), inet_ntoa(addr));
}

char *sw_pretty_port(uint16_t port) {
  char *b = sw_pretty_get_buffer();
  sprintf(b, "%d", port);
  return b;
}

char *sw_pretty_proto(uint16_t protocol) {
  struct protoent *pent = protocol & 0xff00 ? NULL : \
    getprotobynumber(protocol);
  char *b = sw_pretty_get_buffer();
  if (pent) {
    return strcpy(b, pent->p_name);
  } else {
    sprintf(b, "%d", protocol);
    return b;
  }
}

char *sw_pretty_flags(int flags) {
  char *b = sw_pretty_get_buffer();
  int bit = 0x8000;
  strcpy(b, "");
  while(bit) {
    switch(bit&flags) {
    case 0:
      break;
    case SW_FILTER_FLAGS_FIN:
      strcat(b, "F");
      break;
    case SW_FILTER_FLAGS_SYN:
      strcat(b, "S");
      break;
    case SW_FILTER_FLAGS_RST:
      strcat(b, "R");
      break;
    case SW_FILTER_FLAGS_PUSH:
      strcat(b, "P");
      break;
    case SW_FILTER_FLAGS_ACK:
      strcat(b, "A");
      break;
    case SW_FILTER_FLAGS_URG:
      strcat(b, "U");
      break;
    default:
      strcat(b, "?");
      break;
    }
    bit >>= 1;
  }
  return b;
}

/* XXX
getprotobynumber stops working
*/
