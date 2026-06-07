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
#include "acct_scope_event.h"
#include "acct_method_detail.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __commit(char *buffer, size_t length,
                    sw_acct_scope_entry_t *s, time_t delay) {
  sw_acct_scope_event_entry_t *prv = s->prv;

  switch(prv->type) {
  case SW_ACCT_TYPE_START:
    strcat(buffer+strlen(buffer), "\tAcct-Status-Type = Start\n");
    break;
  case SW_ACCT_TYPE_STOP:
  case SW_ACCT_TYPE_STOP_TIME:
    strcat(buffer+strlen(buffer), "\tAcct-Status-Type = Stop\n");
    break;
  case SW_ACCT_TYPE_INTERIM:
    strcat(buffer+strlen(buffer), "\tAcct-Status-Type = Interim-Update\n");
    break;
  }

  sprintf(buffer+strlen(buffer), "\tEvent-Timestamp = %lu\n", prv->timestamp);

  if (prv->event_context && *prv->event_context)
    sprintf(buffer+strlen(buffer), "\tSweet-Event-Context = \"%s\"\n",
            prv->event_context);

  return sw_acct_method_detail_commit(buffer+strlen(buffer),
                                      length-strlen(buffer),
                                      s->next,
                                      delay);
}

sw_acct_method_detail_handler_t acct_method_detail_event = {
  "EVENT",
  -1, /* Module ID placeholder */
  __commit
};
