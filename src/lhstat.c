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
#include <string.h>
#include <errno.h>
#include "lhstat.h"
#include "cfg.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static sw_lhstat_data_t *global_lhstat;


int sw_lhstat_create() {
  global_lhstat = (sw_lhstat_data_t *)0;
  if (sw_debug_flags & SW_DEBUG_LHSTAT)
    sw_log("%s:%d lhash statistics reporting interval %d secs",
           __FILE__, __LINE__, SW_LHSTAT_INTERVAL);
  return 0;
}

int sw_lhstat_new(CONST char *name, LHASH *lh) {
  sw_lhstat_data_t *lhstat;

  lhstat = malloc(sizeof(sw_lhstat_data_t));
  if (lhstat == NULL) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memset(lhstat, 0, sizeof(sw_lhstat_data_t));

  lhstat->name = strdup(name);
  lhstat->lh = lh;

  if (global_lhstat) {
    lhstat->next = global_lhstat;
  }
  global_lhstat = lhstat;
   
  return 0;
}

int sw_lhstat_do(time_t now) {
  sw_lhstat_data_t *lhstat;
  
  for(lhstat=global_lhstat; lhstat; lhstat=lhstat->next) {
    if (!lhstat->last) {
      lhstat->last = now - SW_LHSTAT_INTERVAL/2; /* First time */
    }
    if (now - lhstat->last < SW_LHSTAT_INTERVAL) {
      continue;
    }
    if (sw_debug_flags & SW_DEBUG_LHSTAT)
      sw_log("%s:%d id %s items %lu nodes %lu add %lu (%lu) del %lu (%lu) get %lu (%lu) exp %lu (%lu) cnt %lu (%lu) hash %lu/%lu (%lu/%lu) col %lu (%lu) cmp %lu (%lu), crassig (%d)",
             __FILE__, __LINE__, 
             lhstat->name,
             lhstat->lh->num_items,
             lhstat->lh->num_nodes,
             lhstat->lh->num_insert,
             lhstat->lh->num_insert - lhstat->num_insert,
             lhstat->lh->num_delete,
             lhstat->lh->num_delete - lhstat->num_delete,
             lhstat->lh->num_retrieve,
             lhstat->lh->num_retrieve - lhstat->num_retrieve,
             lhstat->lh->num_expands,
             lhstat->lh->num_expands - lhstat->num_expands,
             lhstat->lh->num_contracts,
             lhstat->lh->num_contracts - lhstat->num_contracts,
             lhstat->lh->num_hash_comps,
             lhstat->lh->num_hash_calls,
             lhstat->lh->num_hash_comps - lhstat->num_hash_comps,
             lhstat->lh->num_hash_calls - lhstat->num_hash_calls,
             lhstat->lh->num_hash_calls ? lhstat->lh->num_hash_comps / lhstat->lh->num_hash_calls : 0,
             (lhstat->lh->num_hash_calls-lhstat->num_hash_calls) ? (lhstat->lh->num_hash_comps-lhstat->num_hash_comps) / (lhstat->lh->num_hash_calls-lhstat->num_hash_calls) : 0,
             lhstat->lh->num_comp_calls,
             lhstat->lh->num_comp_calls - lhstat->num_comp_calls,
             lhstat->lh->num_hash_node_cross_accs);

    /* Update previous-run values */
    lhstat->num_expands = lhstat->lh->num_expands;
    lhstat->num_contracts = lhstat->lh->num_contracts;
    lhstat->num_insert = lhstat->lh->num_insert;
    lhstat->num_delete = lhstat->lh->num_delete;
    lhstat->num_retrieve = lhstat->lh->num_retrieve;
    lhstat->num_hash_calls = lhstat->lh->num_hash_calls;
    lhstat->num_hash_comps = lhstat->lh->num_hash_comps;
    lhstat->num_comp_calls = lhstat->lh->num_comp_calls;
    lhstat->last = now;
  }
  return 0;
}

int sw_lhstat_del(LHASH * lh) {
  sw_lhstat_data_t *lhstat, *lhstat_prev;

  lhstat_prev = lhstat = global_lhstat;
  while (lhstat) {
    if (lhstat->lh == lh) {
      if (lhstat == global_lhstat) {
        global_lhstat = lhstat->next;
      } else {
        lhstat_prev->next = lhstat->next;
      }
      free(lhstat->name);
      free(lhstat);
      return 0;
    } else {
      lhstat_prev = lhstat;
      lhstat = lhstat->next;
    }
  }

  return -1;
}

int sw_lhstat_destroy() {
  sw_lhstat_data_t *lhstat;

  while (global_lhstat) {
    lhstat = global_lhstat;
    global_lhstat = global_lhstat->next;
    free(lhstat->name);
    free(lhstat);
  }

  return 0;
}
