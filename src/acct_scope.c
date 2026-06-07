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
#include <errno.h>
#include <string.h>
#include "pretty.h"
#include "acct_scope.h"
#include "acct_scope_event.h"
#include "acct_scope_session.h"
#include "acct_scope_gauge.h"
#include "acct_scope_snat.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

sw_acct_scope_handler_t *global_acct_scope_handlers[] = {
  &acct_scope_event,
  &acct_scope_session,
  &acct_scope_gauge,
  &acct_scope_snat,
  NULL
};

int sw_acct_scope_create() {
  int idx;
  for(idx=0;global_acct_scope_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_ACCT)
      sw_log("%s:%d create scope handler %s (%d)", __FILE__, __LINE__,
             global_acct_scope_handlers[idx]->name, idx);
    global_acct_scope_handlers[idx]->id = idx;
  }
  return 0;
}

int sw_acct_scope_new(sw_acct_scope_entry_t **s, uint32_t ip,
                      int type, char *event_context) {
  sw_acct_scope_entry_t *p, *t;
  int idx;

  *s = NULL;

  for(idx=0;global_acct_scope_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_ACCT)
      sw_log("%s:%d new scope %s", __FILE__, __LINE__,
             global_acct_scope_handlers[idx]->name);

    p = malloc(sizeof(sw_acct_scope_entry_t));
    if (!p) {
      sw_log("%s:%d malloc() failed: %s",
             __FILE__, __LINE__, strerror(errno));
      return -1;
    }
    memset(p, 0, sizeof(sw_acct_scope_entry_t));

    if (global_acct_scope_handlers[idx]->new &&
        global_acct_scope_handlers[idx]->new(p, ip, type, 
                                             event_context) == -1) {
      free(p);
      return -1;
    }

    p->id = global_acct_scope_handlers[idx]->id;

    if (*s) {
      for(t=*s;t->next;t=t->next);
      t->next = p;        
    } else {
      *s = p;
    }
  }

  return 0;
}

char *sw_acct_scope_pretty(sw_acct_scope_entry_t *s) {
  char *b = sw_pretty_get_buffer();

  for(;s;s = s->next) {
    if (s->id > sizeof(global_acct_scope_handlers)/sizeof(sw_acct_scope_handler_t *)-2) {
      sw_log("%s:%d screwed scope id %d", __FILE__, __LINE__, s->id);
      return "?";
    }

    if (sw_debug_flags & SW_DEBUG_ACCT)
      sw_log("%s:%d make pretty scope %d", __FILE__, __LINE__, s->id);

    strncat(b, global_acct_scope_handlers[s->id]->pretty(s),
            SW_PRETTY_BUFFERS_SIZE);
  }

  return b;
}

int sw_acct_scope_free(sw_acct_scope_entry_t *s) {
  int rc;

  if (!s) {
    return 0;
  }

  if (s->id >= sizeof(global_acct_scope_handlers)) {
    sw_log("%s:%d screwed scope id %d", __FILE__, __LINE__, s->id);
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d free scope %s", __FILE__, __LINE__,
           global_acct_scope_handlers[s->id]->name);

  rc = global_acct_scope_handlers[s->id]->del(s);

  free(s);

  return rc;
}
