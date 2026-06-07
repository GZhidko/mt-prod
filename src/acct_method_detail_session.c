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
#include "session.h"
#include "acct.h"
#include "acct_scope.h"
#include "acct_scope_session.h"
#include "acct_method_detail.h"
#include "pretty.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __commit(char *buffer, size_t length,
                    sw_acct_scope_entry_t *s, time_t delay) {
  sw_acct_scope_session_entry_t *prv = s->prv;

  sprintf(buffer+strlen(buffer), "\tAcct-Session-Id = \"%8.8lX\"\n",
          prv->session_id);
  sprintf(buffer+strlen(buffer), "\tService-Type = Login\n");
  sprintf(buffer+strlen(buffer), "\tFramed-IP-Address = %s\n",
          sw_pretty_ip(prv->ip));
  if (prv->filter_name && *prv->filter_name) {
    sprintf(buffer+strlen(buffer), "\tFilter-Id = \"%s\"\n", prv->filter_name);
  }

  if (prv->type == SW_ACCT_TYPE_STOP || prv->type == SW_ACCT_TYPE_STOP_TIME) {
    sprintf(buffer+strlen(buffer), "\tAcct-Terminate-Cause = ");
    switch(prv->termination_cause) {
    case SW_SESSION_TERM_IDLE:
      sprintf(buffer+strlen(buffer), "Idle-Timeout\n");
      break;
    case SW_SESSION_TERM_TIME:
      sprintf(buffer+strlen(buffer), "Session-Timeout\n");
      break;
    case SW_SESSION_TERM_VOLUME:
      sprintf(buffer+strlen(buffer), "Service-Unavailable\n");
      sprintf(buffer+strlen(buffer), "\tSweet-Session-Terminate-Cause = Traffic-Exhausted\n");
      break;
    case SW_SESSION_TERM_ADMIN:
      sprintf(buffer+strlen(buffer), "Admin-Reset\n");
      break;
    case SW_SESSION_TERM_USER:
      sprintf(buffer+strlen(buffer), "User-Request\n");
      break;
    default:
      sprintf(buffer+strlen(buffer), "NAS-Error\n");
      break;
    }
  }

  if (prv->session_context)
    sprintf(buffer+strlen(buffer), "\tSweet-Session-Context = \"%s\"\n",
            prv->session_context);

  return sw_acct_method_detail_commit(buffer+strlen(buffer),
                                      length-strlen(buffer),
                                      s->next,
                                      delay);
}

sw_acct_method_detail_handler_t acct_method_detail_session = {
  "SESSION",
  -1, /* Module ID placeholder */
  __commit
};
