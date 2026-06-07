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
#include "pretty.h"
#include "acct.h"
#include "acct_method.h"
#include "acct_method_detail.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Acct methods registration */
sw_acct_method_handler_t *global_acct_method_handlers[] = {
  &acct_method_detail,
  NULL
};

static int global_acct_method_handlers_mask = 0;

int sw_acct_method_create() {
  int idx, id = 1;
  for(idx=0;global_acct_method_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_ACCT) 
      sw_log("%s:%d creating %s (%x) method", __FILE__, __LINE__, 
             global_acct_method_handlers[idx]->name, id);

    global_acct_method_handlers[idx]->id = id;
    global_acct_method_handlers_mask |= id;

    id <<= 1;

    if (global_acct_method_handlers[idx]->create &&
        global_acct_method_handlers[idx]->create() == -1) {
      sw_log("%s:%d create() for %s acct method failed", 
             __FILE__, __LINE__, global_acct_method_handlers[idx]->name);
      return -1;
    }
  }

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d all acct methods mask %X", 
           __FILE__, __LINE__, global_acct_method_handlers_mask);

  return 0;
}

int sw_acct_method_destroy() {
  int idx;
  for(idx=0;global_acct_method_handlers[idx];idx++) {
    if (sw_debug_flags & SW_DEBUG_ACCT) 
      sw_log("%s:%d destroying %s (%x) method", __FILE__, __LINE__, 
             global_acct_method_handlers[idx]->name, global_acct_method_handlers[idx]->id);

    if (global_acct_method_handlers[idx]->destroy &&
        global_acct_method_handlers[idx]->destroy() == -1) {
      sw_log("%s:%d destroy() for %s acct method failed", 
             __FILE__, __LINE__, global_acct_method_handlers[idx]->name);
    }
  }
  return 0;
}

int sw_acct_method_commit(sw_acct_entry_t *e, int final_flag) {
  time_t now = time(NULL), delay;
  int idx;

  if (sw_debug_flags & SW_DEBUG_ACCT)
    sw_log("%s:%d processing acct entry %s", 
           __FILE__, __LINE__, sw_acct_scope_pretty(e->scope));

  delay = now-e->record_created;

  for(idx=0;global_acct_method_handlers[idx]; idx++) {
    if (e->method_map & global_acct_method_handlers[idx]->id) {
      if (sw_debug_flags & SW_DEBUG_ACCT)
        sw_log("%s:%d acct %s already fed to %s",
               __FILE__, __LINE__, sw_acct_scope_pretty(e->scope),
               global_acct_method_handlers[idx]->name);
    } else {
      if (sw_debug_flags & SW_DEBUG_ACCT)
        sw_log("%s:%d committing acct %s to %s method", 
               __FILE__, __LINE__, sw_acct_scope_pretty(e->scope),
               global_acct_method_handlers[idx]->name);
      
      if (global_acct_method_handlers[idx]->commit) {
        switch(global_acct_method_handlers[idx]->commit(e, delay)) {
        case -1:
          sw_log("%s:%d %s::commit() failed", __FILE__, __LINE__, 
                 global_acct_method_handlers[idx]->name);
          break;
        case SW_ACCT_METHOD_RC_OK:
          e->method_map |= global_acct_method_handlers[idx]->id;
          break;
        case SW_ACCT_METHOD_RC_LATER:
          break;
        default:
          sw_log("%s:%d unexpected %s::commit() return", 
                 __FILE__, __LINE__, global_acct_method_handlers[idx]->name);
          break;
        }
        if (final_flag && !(e->method_map & global_acct_method_handlers[idx]->id)) {
          sw_log("%s:%d loosing acct %s at %s", __FILE__, __LINE__,
                 sw_acct_scope_pretty(e->scope),
                 global_acct_method_handlers[idx]->name);
          e->method_map |= global_acct_method_handlers[idx]->id;            
        }
      } else {
        /* No commit handler */
        e->method_map |= global_acct_method_handlers[idx]->id;
      }
    }
  }
  
  if (e->method_map == global_acct_method_handlers_mask) {
    if (sw_debug_flags & SW_DEBUG_ACCT)
      sw_log("%s:%d finished all methods for acct entry %s", 
             __FILE__, __LINE__, sw_acct_scope_pretty(e->scope));
    return SW_ACCT_METHOD_RC_OK;
  }

  return SW_ACCT_METHOD_RC_LATER;
}
