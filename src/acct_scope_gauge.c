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
#include "gauge.h"
#include "acct.h"
#include "acct_scope_gauge.h"
#include "pretty.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __new(sw_acct_scope_entry_t *s, uint32_t ip, int type,
                 char *event_context) {
  sw_acct_scope_gauge_entry_t *prv;

  prv = (sw_acct_scope_gauge_entry_t *)malloc(sizeof(sw_acct_scope_gauge_entry_t));
  if (!prv) {
    sw_log("%s:%d malloc() failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memset(prv, 0, sizeof(sw_acct_scope_gauge_entry_t));

  if (sw_gauge_limits(ip, &prv->max_octets_in, &prv->max_octets_out,
                       &prv->max_bps_in, &prv->max_bps_out,
                       &prv->max_duration, &prv->max_idle,(SW_ACCT_TYPE_STOP_TIME!=type)) == -1) {
    free(prv);
    return -1;
  }

  if (type == SW_ACCT_TYPE_INTERIM || type == SW_ACCT_TYPE_STOP || prv->type == SW_ACCT_TYPE_STOP_TIME) {
    if (sw_gauge_current(ip, &prv->octets_in, &prv->octets_out,
                         &prv->packets_in, &prv->packets_out,
                         NULL, NULL,
                         &prv->duration, &prv->idle,(SW_ACCT_TYPE_STOP_TIME!=type)) == -1 ||
        sw_gauge_peak(ip, &prv->bps_in, &prv->bps_out,(SW_ACCT_TYPE_STOP_TIME!=type)) == -1) {
      free(prv);
      return -1;
    }
  }

  prv->type = type;

  s->prv = prv;

  return 0;
}

static char *__pretty(sw_acct_scope_entry_t *s) {
  return "";
}

static int __free(sw_acct_scope_entry_t *s) {
  sw_acct_scope_gauge_entry_t *prv = s->prv;
  if (prv) {
    free(prv);
  }
  return sw_acct_scope_free(s->next);
}

sw_acct_scope_handler_t acct_scope_gauge = {
  "GAUGE",
  -1, /* Module ID placeholder */
  __new,
  __pretty,
  __free
};
