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
#include <time.h>
#include <sys/select.h>
#include <pthread.h>
#include "pretty.h"
#include "acct.h"
#include "acct_method.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static sw_acct_entry_t *global_acct_chain = NULL;
static pthread_mutex_t global_acct_lock = PTHREAD_MUTEX_INITIALIZER;

static int sw_acct_init_scope(sw_acct_entry_t *e) {
  if (e->scope) {
    return 0;
  }

  if (sw_acct_scope_new(&e->scope, e->ip, e->type,
                        e->event_context ? e->event_context : "") == -1) {
    return -1;
  }

  return 0;
}

int sw_acct_commit(int final_flag) {
  sw_acct_entry_t *e, *next, *retry_head, *retry_tail;

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d run acct queue", __FILE__, __LINE__);

  pthread_mutex_lock(&global_acct_lock);
  e = global_acct_chain;
  global_acct_chain = NULL;
  pthread_mutex_unlock(&global_acct_lock);
  retry_head = retry_tail = NULL;

  while(e) {
    next = e->next;
    e->next = NULL;

    if (sw_acct_init_scope(e) == -1) {
      if (!final_flag) {
        if (retry_tail) {
          retry_tail->next = e;
        } else {
          retry_head = e;
        }
        retry_tail = e;
      } else {
        if (e->event_context) {
          free(e->event_context);
        }
        free(e);
      }
      e = next;
      continue;
    }

    if (sw_debug_flags & SW_DEBUG_ACCT)
      sw_log("%s:%d processing acct entry %s", 
             __FILE__, __LINE__, sw_acct_scope_pretty(e->scope));

    
    switch(sw_acct_method_commit(e, final_flag)) {
	    case SW_ACCT_METHOD_RC_OK:
      if (sw_debug_flags & SW_DEBUG_ACCT)
        sw_log("%s:%d acct %s done", __FILE__, __LINE__, 
               sw_acct_scope_pretty(e->scope));
	      sw_acct_scope_free(e->scope);
	      if (e->event_context) {
	        free(e->event_context);
	      }
	      free(e);
	      break;
	    case SW_ACCT_METHOD_RC_LATER:
          if (retry_tail) {
            retry_tail->next = e;
          } else {
            retry_head = e;
          }
          retry_tail = e;
	      break;
	    default:
	      return -1;
	    }
    e = next;
  }

  if (retry_head) {
    pthread_mutex_lock(&global_acct_lock);
    retry_tail->next = global_acct_chain;
    global_acct_chain = retry_head;
    pthread_mutex_unlock(&global_acct_lock);
  }

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d finished acct queue", __FILE__, __LINE__);

  return 0;
}

int sw_acct_submit(uint32_t ip, int type, char *event_context) {
  sw_acct_entry_t *e, *p;

  e = malloc(sizeof(sw_acct_entry_t));
  if (!e) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memset(e, 0, sizeof(sw_acct_entry_t));

  e->ip = ip;
  e->type = type;
  e->record_created = time(NULL);
  if (event_context && *event_context) {
    e->event_context = strdup(event_context);
    if (!e->event_context) {
      free(e);
      return -1;
    }
  }

  pthread_mutex_lock(&global_acct_lock);
  for(p = global_acct_chain; p && p->next; p = p->next);
  if (p) {
    p->next = e;
  } else {
    global_acct_chain = e;
  }
  pthread_mutex_unlock(&global_acct_lock);

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d acct ip=%s type=%d submitted", __FILE__, __LINE__,
           sw_pretty_ip(ip), type);

  return 0;
}
