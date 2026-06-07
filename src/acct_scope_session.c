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
#include "session.h"
#include "acct.h"
#include "acct_scope_session.h"
#include "pretty.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __new(sw_acct_scope_entry_t *s, uint32_t ip, int type,
                 char *event_context) {
  sw_acct_scope_session_entry_t *prv;

  prv = (sw_acct_scope_session_entry_t *)malloc(sizeof(sw_acct_scope_session_entry_t));
  if (!prv) {
    sw_log("%s:%d malloc() failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memset(prv, 0, sizeof(sw_acct_scope_session_entry_t));

  if (sw_session_status_nonblock(ip, NULL, &prv->session_id,
                        &prv->filter_name, NULL, &prv->session_context,
                        &prv->termination_cause) == -1) {
    free(prv);
    return -1;
  }

  if (prv->session_context)
    prv->session_context = strdup(prv->session_context);
  if (prv->filter_name)
    prv->filter_name = strdup(prv->filter_name);

  prv->type = type;
  prv->ip = ip;

  s->prv = prv;

  return 0;
}

static char *__pretty(sw_acct_scope_entry_t *s) {
  sw_acct_scope_session_entry_t *prv = s->prv;
  char *b = sw_pretty_get_buffer();
  sprintf(b, "%8.8lX", prv->session_id);
  return b;
}

static int __free(sw_acct_scope_entry_t *s) {
  sw_acct_scope_session_entry_t *prv = s->prv;
  if (prv) {
    if (prv->session_context)
      free(prv->session_context);
    if (prv->filter_name)
      free(prv->filter_name);
    free(prv);
  }
  return sw_acct_scope_free(s->next);
}

sw_acct_scope_handler_t acct_scope_session = {
  "SESSION",
  -1, /* Module ID placeholder */
  __new,
  __pretty,
  __free
};
