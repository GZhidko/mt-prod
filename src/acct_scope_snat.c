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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "acct.h"
#include "acct_scope_snat.h"
#include "nat_src.h"
#include "nat_src_endpoint_ip.h"
#include "nat_src_endpoint_udp.h"
#include "nat_src_endpoint_tcp.h"
#include "nat_src_endpoint_icmp.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __new(sw_acct_scope_entry_t *s, uint32_t ip, int type,
                 char *event_context) {
  sw_acct_scope_snat_entry_t *prv;

  if (!sw_nat_src_enabled_flag) {
    s->prv = NULL;
    return 0;
  }

  prv = (sw_acct_scope_snat_entry_t *)malloc(sizeof(sw_acct_scope_snat_entry_t));
  if (!prv) {
    sw_log("%s:%d malloc() failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_nat_src_endpoint_ip_getpub(&prv->ip, ip) == -1) {
    free(prv);
    return -1;
  }

  if (sw_nat_src_endpoint_udp_getpub(&prv->udp_lport,
                                     &prv->udp_hport, ip) == -1) {
    free(prv);
    return -1;
  }

  if (sw_nat_src_endpoint_tcp_getpub(&prv->tcp_lport, 
                                     &prv->tcp_hport, ip) == -1) {
    free(prv);
    return -1;
  }

  if (sw_nat_src_endpoint_icmp_getpub(&prv->icmp_lseq, 
                                      &prv->icmp_hseq, ip) == -1) {
    free(prv);
    return -1;
  }

  s->prv = prv;

  return 0;
}

static char *__pretty(sw_acct_scope_entry_t *s) {
  return "";
}

static int __free(sw_acct_scope_entry_t *s) {
  sw_acct_scope_snat_entry_t *prv = s->prv;
  if (prv) {
    free(prv);
  }
  return sw_acct_scope_free(s->next);
}

sw_acct_scope_handler_t acct_scope_snat = {
  "SNAT",
  -1, /* Module ID placeholder */
  __new,
  __pretty,
  __free,
};
