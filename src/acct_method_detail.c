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
#include "acct_method_detail.h"
#include "acct_method_detail_event.h"
#include "acct_method_detail_session.h"
#include "acct_method_detail_gauge.h"
#include "acct_method_detail_snat.h"
#include "pretty.h"
#include "cfg.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

sw_acct_method_detail_handler_t *global_acct_method_detail_handlers[] = {
  /* Modules registration order must be aligned with acct_scopes */
  &acct_method_detail_event,
  &acct_method_detail_session,
  &acct_method_detail_gauge,
  &acct_method_detail_snat,
  NULL
};

int sw_acct_method_detail_create() {
  int idx;
  for(idx=0;global_acct_method_detail_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_ACCT)
      sw_log("%s:%d create detail sub-method %s (%d)", __FILE__, __LINE__,
             global_acct_method_detail_handlers[idx]->name, idx);
    global_acct_method_detail_handlers[idx]->id = idx;
  }
  return 0;
}

int sw_acct_method_detail_commit(char *buffer, size_t length,
                                 sw_acct_scope_entry_t *s, time_t delay) {
  if (!s) {
    return 0;
  }

  if (s->id > sizeof(global_acct_method_detail_handlers)/sizeof(sw_acct_method_detail_handler_t *)-2) {
    sw_log("%s:%d screwed detail sub-method id %d", __FILE__, __LINE__, s->id);
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d calling %s", __FILE__, __LINE__,
           global_acct_method_detail_handlers[s->id]->name);

  return global_acct_method_detail_handlers[s->id]->commit(buffer, length, s, delay);
}

static int __commit(sw_acct_entry_t *e, time_t delay) {
  static CONST char *detail = NULL;
  static char b[65535];
  time_t now = time(NULL);
  int fd;

  if (detail == NULL) {
    detail = sw_config_get("acct-detail-file", "/var/sweetspot/detail");
  }

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d storing %s in %s", __FILE__, __LINE__, 
           sw_acct_scope_pretty(e->scope), detail);

  sprintf(b, "%s", asctime(localtime(&now)));

  if (sw_acct_method_detail_commit(b+strlen(b), sizeof(b)-strlen(b)-1,
                                   e->scope, delay) == -1) {
    return -1;
  }

  strcat(b, "\n");

  fd = open(detail, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
  if (fd == -1) {
    sw_log("%s:%d open() failed: %s", __FILE__, __LINE__, 
           strerror(errno));    
    return SW_ACCT_RC_LATER;
  }

  if (write(fd, b, strlen(b)) != strlen(b)) {
    sw_log("%s:%d write() to %s failed: %s", __FILE__, __LINE__, 
           detail, strerror(errno));    
    close(fd);
    return SW_ACCT_RC_LATER;
  }

  close(fd);

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d done with %s", __FILE__, __LINE__, 
           sw_acct_scope_pretty(e->scope));

  return SW_ACCT_RC_OK;
}

sw_acct_method_handler_t acct_method_detail = {
  "DETAIL",
  0, /* method ID placeholder */
  NULL, /* create() */
  NULL, /* destroy() */
  &__commit
};
