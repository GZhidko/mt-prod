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
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "acct.h"
#include "acct_scope.h"
#include "acct_scope_snat.h"
#include "acct_method_detail.h"
#include "pretty.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __commit(char *buffer, size_t length,
                    sw_acct_scope_entry_t *s, time_t delay) {
  sw_acct_scope_snat_entry_t *prv = s->prv;

  if (prv) {
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-IP-Address = %s\n", 
            sw_pretty_ip(prv->ip));
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-TCP-Port-Low = %s\n", 
            sw_pretty_port(prv->tcp_lport));
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-TCP-Port-High = %s\n", 
            sw_pretty_port(prv->tcp_hport));
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-UDP-Port-Low = %s\n", 
            sw_pretty_port(prv->udp_lport));
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-UDP-Port-High = %s\n", 
            sw_pretty_port(prv->udp_hport));
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-ICMP-Seq-Low = %d\n",
            prv->icmp_lseq);
    sprintf(buffer+strlen(buffer), "\tSweet-SNAT-ICMP-Seq-High = %d\n",
            prv->icmp_hseq);
  }

  return sw_acct_method_detail_commit(buffer+strlen(buffer),
                                      length-strlen(buffer),
                                      s->next,
                                      delay);
}

sw_acct_method_detail_handler_t acct_method_detail_snat = {
  "SNAT",
  -1, /* Module ID placeholder */
  __commit
};
