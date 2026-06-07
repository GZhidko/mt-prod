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
#include "gauge.h"
#include "acct.h"
#include "acct_scope.h"
#include "acct_scope_gauge.h"
#include "acct_method_detail.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __commit(char *buffer, size_t length,
                    sw_acct_scope_entry_t *s, time_t delay) {
  sw_acct_scope_gauge_entry_t *prv = s->prv;

  sprintf(buffer+strlen(buffer), "\tAcct-Delay-Time = %lu\n",
          delay);
  if (prv->type == SW_ACCT_TYPE_INTERIM || prv->type == SW_ACCT_TYPE_STOP || prv->type == SW_ACCT_TYPE_STOP_TIME) {
    sprintf(buffer+strlen(buffer), "\tAcct-Input-Octets = %lu\n",
            (uint32_t)(0xffffffffl & prv->octets_in));
    sprintf(buffer+strlen(buffer), "\tAcct-Output-Octets = %lu\n", 
            (uint32_t)(0xffffffffl & prv->octets_out));
    if (prv->octets_in & 0xffffffffl) {
      sprintf(buffer+strlen(buffer), "\tAcct-Input-Gigawords = %lu\n",
              (uint32_t)(prv->octets_in >> 32));
    }
    if (prv->octets_out & 0xffffffffl) {
      sprintf(buffer+strlen(buffer), "\tAcct-Output-Gigawords = %lu\n", 
              (uint32_t)(prv->octets_out >> 32));
    }
    sprintf(buffer+strlen(buffer), "\tAcct-Input-Packets = %llu\n",
            0xffffffffl & prv->packets_in);
    sprintf(buffer+strlen(buffer), "\tAcct-Output-Packets = %llu\n", 
            0xffffffffl & prv->packets_out);

    sprintf(buffer+strlen(buffer), "\tBandwidth-Max-Up = %llu\n",
            prv->bps_in);
    sprintf(buffer+strlen(buffer), "\tBandwidth-Max-Down = %llu\n",
            prv->bps_out);

    sprintf(buffer+strlen(buffer), "\tAcct-Session-Time = %lu\n", 
            prv->duration - prv->idle);
  }

  if (prv->max_octets_in != SW_GAUGE_MAX_OCTETS_IN) {
    sprintf(buffer+strlen(buffer), "\tSweet-Max-Octets-In = \"%llu\"\n",
            prv->max_octets_in);
  }
  if (prv->max_octets_out != SW_GAUGE_MAX_OCTETS_OUT) {
    sprintf(buffer+strlen(buffer), "\tSweet-Max-Octets-Out = \"%llu\"\n", 
            prv->max_octets_out);
  }
  if (prv->max_bps_in != SW_GAUGE_MAX_BPS_IN) {
    sprintf(buffer+strlen(buffer), "\tSweet-Max-Bps-Up = \"%llu\"\n",
            prv->max_bps_in);
  }
  if (prv->max_bps_out != SW_GAUGE_MAX_BPS_OUT) {
    sprintf(buffer+strlen(buffer), "\tSweet-Max-Bps-Down = \"%llu\"\n",
            prv->max_bps_out);
  }
  sprintf(buffer+strlen(buffer), "\tSweet-Max-Duration = %lu\n", 
          prv->max_duration);
  sprintf(buffer+strlen(buffer), "\tSweet-Max-Idle = %lu\n", 
          prv->max_idle);

  return sw_acct_method_detail_commit(buffer+strlen(buffer),
                                      length-strlen(buffer),
                                      s->next,
                                      delay);
}

sw_acct_method_detail_handler_t acct_method_detail_gauge = {
  "GAUGE",
  -1, /* Module ID placeholder */
  __commit
};
