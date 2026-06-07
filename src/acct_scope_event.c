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
#include "acct_scope_event.h"
#include "pretty.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int __new(sw_acct_scope_entry_t *s, uint32_t ip, int type,
                 char *event_context) {
  sw_acct_scope_event_entry_t *prv;

  prv = (sw_acct_scope_event_entry_t *)malloc(sizeof(sw_acct_scope_event_entry_t));
  if (!prv) {
    sw_log("%s:%d malloc() failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  prv->type = type;
  prv->timestamp = time(NULL);
  prv->event_context = strdup(event_context);

  s->prv = prv;

  return 0;
}

static char *__pretty(sw_acct_scope_entry_t *s) {
  sw_acct_scope_event_entry_t *prv = s->prv;
  char *b = sw_pretty_get_buffer();
  sprintf(b, "%d:", prv->type);
  return b;
}

static int __free(sw_acct_scope_entry_t *s) {
  sw_acct_scope_event_entry_t *prv = s->prv;
  if (prv) {
    free(prv->event_context);
    free(prv);
  }
  return sw_acct_scope_free(s->next);
}

sw_acct_scope_handler_t acct_scope_event = {
  "EVENT",
  -1, /* Module ID placeholder */
  __new,
  __pretty,
  __free
};
