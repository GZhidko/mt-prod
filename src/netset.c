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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include "pretty.h"
#include "netset.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

int sw_netset_create(sw_netset_t **nethead, CONST char *cidrs) {
  sw_netset_t *netset;
  struct in_addr ip;
  uint32_t netmask, ip_min, ip_max;
  char buf[8192], *cidr;

  *nethead = netset = NULL;

  if (strlen(cidrs) >= sizeof(buf)) {
    sw_log("%s:%d enlarge you know what", __FILE__, __LINE__);
    return -1;
  }
  strcpy(buf, cidrs);
  
  cidr = strtok(buf, " \t:,");
  while(cidr) {
    char *a;
    int n;

    a = cidr;
    while(*a && *a != '/') a++;
    if (*a == '/') {
      n = atol(a+1);
      *a = '\0';
    } else {
      n = 32;
    }

    if (inet_aton(cidr, &ip) == 0) {
      sw_log("%s:%d inet_aton() failed for %s: %s", 
             __FILE__, __LINE__, cidr, strerror(errno));
      return -1;
    }
    ip_min = ntohl(ip.s_addr);

    netmask = 0xffffffff << (32-n);
    if (!netmask) {
      sw_log("%s:%d lame netmask %s", __FILE__, __LINE__, n);
      return -1;
    }
    ip_max = ~netmask | ip_min;

    if (sw_debug_flags & SW_DEBUG_NETSET) 
      sw_log("%s:%d IP range %s->%s", __FILE__, __LINE__, 
             sw_pretty_ip(ip_min), sw_pretty_ip(ip_max));

    cidr = strtok(NULL, " \t:,");

    netset = malloc(sizeof(sw_netset_t));
    if (!netset) {
      sw_log("%s:%d malloc failed", __FILE__, __LINE__, strerror(errno));
      sw_netset_destroy(*nethead);
      return -1;
    }

    netset->ip_min = ip_min;
    netset->ip_max = ip_max;
    netset->next = NULL;

    if (*nethead) {
      netset->next = *nethead;
      *nethead = netset;
    } else {
      *nethead = netset;
    }
  }

  if (sw_debug_flags & SW_DEBUG_NETSET) 
    sw_log("%s:%d networks loaded %s (%d addresses)", __FILE__, __LINE__, 
	   sw_netset_pretty(*nethead), sw_netset_size(*nethead));
	   
  return 0;
}

int sw_netset_destroy(sw_netset_t *nethead) {
    sw_netset_t *n;
    while(nethead) {
      n = nethead;
      nethead = nethead->next;
      free(n);
    }

    return 0;
}

char *sw_netset_pretty(sw_netset_t *nethead) {
  char *b;
  if (!nethead)
    return "";
  b = sw_pretty_get_buffer();
  sprintf(b, "%s-%s,%s", sw_pretty_ip(nethead->ip_min),
	  sw_pretty_ip(nethead->ip_max), sw_netset_pretty(nethead->next));
  return b;
}

uint32_t sw_netset_size(sw_netset_t *nethead) {
  sw_netset_t *ns;
  uint32_t size = 0;
  if (!nethead)
    return 0;
  for(ns=nethead;ns;ns=ns->next)
    size += ns->ip_max - ns->ip_min + 1;
  return size;
}

uint32_t sw_netset_idx(sw_netset_t *nethead, uint32_t ip) {
  sw_netset_t *ns;
  uint32_t ip_idx = 0;
  if (!nethead)
    return 0;
  for(ns=nethead;ns;ns=ns->next) {
    if (ip > ns->ip_max || ip < ns->ip_min) {
      ip_idx += ns->ip_max - ns->ip_min + 1;
    } else {
      ip_idx += ip - ns->ip_min;
      break;
    }
  }
  return ip_idx;
}

uint32_t sw_check_ip(sw_netset_t *nethead, uint32_t ip) {
  sw_netset_t *ns;
  uint32_t ip_idx = 0;
  if (!nethead)
    return 0;
  for(ns=nethead;ns;ns=ns->next) {
    if (ip >= ns->ip_min && ip <= ns->ip_max){
      return 1;
    }
  }
  return ip_idx;
}

uint32_t sw_netset_ip(sw_netset_t *nethead, uint32_t ip_idx) {
  sw_netset_t *ns;
  uint32_t ip = 0;
  if (!nethead)
    return 0;
  for(ns=nethead;ns;ns=ns->next) {
    if (ns->ip_max - ns->ip_min < ip_idx ) {
      ip_idx -= ns->ip_max - ns->ip_min + 1;
    } else {
      ip = ns->ip_min + ip_idx;
      break;
    }
  }
  return ip;
}

