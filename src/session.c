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
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include "lhash.h"
#include "acct.h"
#include "pretty.h"
#include "cfg.h"
#include "filter.h"
#include "session.h"
#include "nat_src.h"
#include "nat_src_endpoint.h"
#include "lhstat.h"
#include "log.h"
#include "if.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

sw_netset_t *global_ns;
sw_netset_t * local_ip_ns = NULL;
sw_netset_t * snat_ip_ns = NULL;

static LHASH *global_session_hash=(LHASH*)0;

static unsigned long __hash_cb(sw_session_entry_t *e) {
  return e->ip;
}

static int __cmp_cb(sw_session_entry_t *e1, sw_session_entry_t *e2) {
  return e1->ip != e2->ip;
}

static void __term_session_cb(sw_session_entry_t *s) {
  sw_session_stop(s->ip, "", SW_SESSION_TERM_ADMIN, "", "");
}

static void __expire_session_cb(sw_session_entry_t *s, time_t *now) {
  switch(s->state) {
  case SW_SESSION_ST_CAPTURED:
    /* Expire session */
    if (s->deathday < *now) {
      if (sw_debug_flags & SW_DEBUG_SESSION)
        sw_log("%s:%d delete captured session %s", __FILE__, __LINE__, 
               sw_pretty_ip(s->ip));
      sw_session_del(s->ip);
    }
    break;
  case SW_SESSION_ST_RELEASED:
    /* Interim accounting */
    if (s->interim < *now) {
      if (sw_debug_flags & (SW_DEBUG_SESSION | SW_DEBUG_ACCT))
        sw_log("%s:%d requesting interim acct for %s", __FILE__, __LINE__, 
               sw_pretty_ip(s->ip));
      sw_acct_submit(s->ip, SW_ACCT_TYPE_INTERIM, "");
      s->interim = *now + s->interim_interval;
    }
    break;
  }
}

static int run_script(CONST char *script, CONST char *cidrs, char *action) {
  char buf[8192], cmd[256], *cidr;;

  cidr = strtok(strcpy(buf, cidrs), " \t:,");
  while(cidr) {
    sprintf(cmd, "%s %s %s", script, cidr, action);
    if (system(cmd) == -1) {
      sw_log("%s:%d system(\"%s\") failed: %s",  __FILE__, __LINE__,
             cmd, strerror(errno));
      return -1;
    }
  
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d system(\"%s\") succeeded",  __FILE__, __LINE__, cmd);
    
    cidr = strtok(NULL, " \t:,");
  }

  return 0;
}

static uint32_t next_session_id() {
  static uint32_t source_session_id = 1;
  uint32_t session_id = __atomic_fetch_add(&source_session_id, 1, __ATOMIC_SEQ_CST);

  if (session_id >= 0x7fffffff) {
    uint32_t expected = session_id + 1;
    __atomic_compare_exchange_n(&source_session_id, &expected, 1, 0,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    session_id = 1;
  }

  return session_id;
}
                        
int sw_session_create() {
  CONST char *user_cidrs = sw_config_get("user-networks", NULL);
  CONST char *snat_cidrs = sw_config_get("snat-public-networks", NULL);
  CONST char *local_ip_cidrs = sw_config_get("local-networks", NULL);

  if (!user_cidrs) {
    sw_log("%s:%d user-networks not configured", __FILE__, __LINE__);
    return -1;
  }



  if (sw_netset_create(&global_ns, user_cidrs) == -1) {
    return -1;
  }

  global_session_hash = lh_new(__hash_cb, __cmp_cb,(sw_netset_size(global_ns)+10)*2);
  if (!global_session_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  sw_lhstat_new("session", global_session_hash);

  if (sw_debug_flags & (SW_DEBUG_SESSION | SW_DEBUG_BREATH))
    sw_log("%s:%d session pool %s", __FILE__, __LINE__,
           sw_netset_pretty(global_ns));

  if (snat_cidrs) {
    if (sw_nat_src_create() == -1) {
      return -1;
    }

    if (sw_nat_src_endpoint_create(global_ns) < 0) {
      return -1;
    }

    sw_nat_src_enabled_flag = 1; /* Enable SNAT */
  }

  if(local_ip_cidrs){
  if (sw_netset_create(&local_ip_ns, local_ip_cidrs) == -1) {
    return -1;
  }
  
 
  sw_log("%s:%d local IP pool %s", __FILE__, __LINE__,
         sw_netset_pretty(local_ip_ns));
}

 if(snat_cidrs)
  if (sw_netset_create(&snat_ip_ns, snat_cidrs) == -1) {
    return -1;
  }




 /* if (run_script(sw_config_get("upstream-nets-rc", SW_SESSION_UPSTREAM_RC),
                 user_cidrs, "start") == -1) {
    return -1;
  }

  if (run_script(sw_config_get("downstream-nets-rc", SW_SESSION_DOWNSTREAM_RC),
                 snat_cidrs ? snat_cidrs : user_cidrs, "start") == -1) {
    return -1;
  }*/

  return 0;
}

int sw_session_destroy() {
  CONST char *user_cidrs = sw_config_get("user-networks", NULL);
  CONST char *snat_cidrs = sw_config_get("snat-public-networks", NULL);

  if (sw_debug_flags & SW_DEBUG_SESSION) 
    sw_log("%s:%d destroying session pool", __FILE__, __LINE__);

  sw_netset_destroy(global_ns);

  sw_lhstat_del(global_session_hash);

  lh_doall(global_session_hash, __term_session_cb, 1);

  lh_free(global_session_hash);

  global_session_hash = (LHASH*)0;

  /*if (run_script(sw_config_get("upstream-nets-rc", SW_SESSION_UPSTREAM_RC),
                 user_cidrs, "stop") == -1) {
    return -1;
  }

  if (run_script(sw_config_get("downstream-nets-rc", SW_SESSION_DOWNSTREAM_RC),
                 snat_cidrs ? snat_cidrs : user_cidrs, "stop") == -1) {
    return -1;
  }*/

  return 0;
}

int sw_session_new(sw_session_entry_t **n, uint32_t ip, spinlock ** sp) {
  sw_netset_t *ns;
  sw_session_entry_t qs, *s;
  //spinlock * sp = NULL;

  qs.ip = ip;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,sp);
  if (s) {
    if (sw_debug_flags & SW_DEBUG_SESSION) 
      sw_log("%s:%d session exists for %s", __FILE__, __LINE__,
	     sw_pretty_ip(ip));
  } else {
    spinlock_unlock(*sp);
    for(ns=global_ns;ns;ns=ns->next) {
      if (ip >= ns->ip_min && ip <= ns->ip_max) {
        if (sw_debug_flags & SW_DEBUG_SESSION) 
          sw_log("%s:%d IP %s in range",  __FILE__, __LINE__, 
                 sw_pretty_ip(ip));
        break;
      }
    }
    if (!ns) {
      if (sw_debug_flags & SW_DEBUG_SESSION) 
        sw_log("%s:%d IP %s out of range",  __FILE__, __LINE__, 
               sw_pretty_ip(ip));
      return -1;
    }
    
    s = malloc(sizeof(sw_session_entry_t));
    if (s == NULL) {
      sw_log("%s:%d malloc() failed for %s", __FILE__, __LINE__,
	     sw_pretty_ip(ip));
      return -1;
    }
    memset(s, 0, sizeof(sw_session_entry_t));
    s->ip = ip;
    s->state = SW_SESSION_ST_CAPTURED;
    
    s->filter_id = sw_filter_get_id(sw_config_get("filter-anonymous", SW_FILTER_NAME_ANON));
    if (s->filter_id == -1) {
      sw_log("%s:%d packet filter %s not found",
	     __FILE__, __LINE__,
	     sw_config_get("filter-anonymous", SW_FILTER_NAME_ANON));
      free(s);
      return -1;
    }

    s->retention = SW_SESSION_DEFAULT_RETENTION;
    s->deathday = time(NULL) + s->retention;
  
    lh_insert(global_session_hash, (void *)s,sp);

    if (sw_nat_src_enabled_flag && 
        sw_nat_src_endpoint_new(global_ns, ip) == -1) {
      sw_session_del(ip);
      spinlock_unlock(*sp);
      return -1;
    }

    s->session_id = next_session_id();

    if (sw_debug_flags & (SW_DEBUG_SESSION | SW_DEBUG_BREATH)) 
      sw_log("%s:%d session %s created", __FILE__, __LINE__, sw_pretty_ip(ip));
  }

  if (n) {
    *n = s;
  }
  
  return 0;
}

int sw_session_del(uint32_t ip) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,NULL);
  if (!s) {
    sw_log("%s:%d no session for %s", __FILE__, __LINE__,sw_pretty_ip(ip));
    return -1;
  }

  if (sw_nat_src_enabled_flag) sw_nat_src_endpoint_del(global_ns, ip);

  lh_delete(global_session_hash, (void *)s,NULL);

  if (s->session_context) free(s->session_context);
  free(s);

  sw_shape_del(ip);

  if (sw_debug_flags & (SW_DEBUG_SESSION | SW_DEBUG_BREATH)) 
    sw_log("%s:%d session %s destroyed", __FILE__, __LINE__, sw_pretty_ip(ip));

  return 0;
}

int sw_session_stop(uint32_t ip, 
                    char *filter_name,
                    int termination_cause,
                    char *session_context,
                    char *event_context) {
  sw_session_entry_t qs, *s;
  int filter_id, prev_state = 0, acct_type = -1;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (s) {
    if (s->state == SW_SESSION_ST_RELEASED) {
      s->state = SW_SESSION_ST_CAPTURED;
      s->termination_cause = termination_cause;
      prev_state = SW_SESSION_ST_RELEASED;
      acct_type = (termination_cause == SW_SESSION_TERM_IDLE ||
                   termination_cause == SW_SESSION_TERM_TIME) ?
                  SW_ACCT_TYPE_STOP_TIME : SW_ACCT_TYPE_STOP;
      s->session_id = next_session_id();
    }
  } else {
    spinlock_unlock(sp);
    if (sw_session_new(&s, ip, &sp) == -1) {
      return -1;
    }
  }

  if (filter_name && *filter_name) {
    if (s->dn_filter_name) free(s->dn_filter_name);
    s->dn_filter_name = strdup(filter_name);
  } else if (!filter_name && s->dn_filter_name) {
    filter_name = s->dn_filter_name;
  } else {
    if (s->dn_filter_name) free(s->dn_filter_name);
    s->dn_filter_name = NULL;
    filter_name = sw_config_get("filter-anonymous", SW_FILTER_NAME_ANON);
  }
  filter_id = sw_filter_get_id(filter_name); 
  if (filter_id == -1) {
      sw_log("%s:%d packet filter \"%s\" not found", __FILE__, __LINE__,
             filter_name);
      spinlock_unlock(sp);
      return -1;
    }
  s->filter_id = filter_id;

  if (sw_debug_flags & (SW_DEBUG_SESSION | SW_DEBUG_BREATH))
    if (prev_state == SW_SESSION_ST_RELEASED)
      sw_log("%s:%d session %s is now CAPTURED (filter %s)",
               __FILE__, __LINE__, sw_pretty_ip(s->ip), filter_name);
    else if (s->dn_filter_name) 
      sw_log("%s:%d session %s is CAPTURED (filter %s)",
               __FILE__, __LINE__, sw_pretty_ip(s->ip), filter_name);

  if (*session_context) {
    if (s->session_context)
      free(s->session_context);
    s->session_context = strdup(session_context);
  }

  s->deathday = time(NULL) + s->retention;
  spinlock_unlock(sp);

  if (acct_type != -1 && sw_acct_submit(ip, acct_type, event_context) == -1) {
    return -1;
  }

  return 0;
}

int sw_session_start(uint32_t ip, 
                     char *filter_name,
                     int interim_interval,
                     char *session_context,
                     char *event_context) {
  sw_session_entry_t qs, *s;
  int need_start_acct = 0;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (!s) {
    spinlock_unlock(sp);
    if (sw_session_new(&s, ip, &sp) == -1) {
      return -1;
    }
  }

  if (*filter_name) {
    int filter_id = sw_filter_get_id(filter_name);
    if (filter_id == -1) {
      sw_log("%s:%d packet filter \"%s\" not found", __FILE__, __LINE__,
             filter_name);
      spinlock_unlock(sp);
      return -1;
    }
    s->filter_id = filter_id;
  } else {
    s->filter_id = SW_FILTER_ID_NONE;
  }

  if (sw_debug_flags & SW_DEBUG_SESSION)
    sw_log("%s:%d using filter \"%s\" (#%d) against %s", __FILE__, __LINE__, 
           filter_name, s->filter_id, sw_pretty_ip(s->ip));

  if (!interim_interval) {
    interim_interval = atoi(sw_config_get("acct-interim-interval",
                                          SW_SESSION_ACCT_INTERIM_INTERVAL));
  }

  if (s->interim) {
    s->interim += interim_interval - s->interim_interval;
  } else {
    s->interim = time(NULL) + interim_interval;
  }
  s->interim_interval = interim_interval;

  if (*session_context) {
    if (s->session_context)
      free(s->session_context);
    s->session_context = strdup(session_context);
  }

  if (s->state == SW_SESSION_ST_CAPTURED) {
    s->state = SW_SESSION_ST_RELEASED;

    if (sw_debug_flags & (SW_DEBUG_SESSION | SW_DEBUG_BREATH))
      sw_log("%s:%d session %s is now RELEASED (filter %s)",
             __FILE__, __LINE__, sw_pretty_ip(s->ip),
             *filter_name ? filter_name : "<none>");

    s->interim = time(NULL) + interim_interval; /* Align interims to Start */
    need_start_acct = 1;
  }
  spinlock_unlock(sp);

  if (need_start_acct && sw_acct_submit(ip, SW_ACCT_TYPE_START, event_context) == -1) {
    return -1;
  }

  return 0;
}

int sw_session_status(uint32_t ip, 
                      int *state,
                      uint32_t *session_id,
                      char **filter_name,
                      time_t *interim_interval,
                      char **session_context,
                      int *termination_cause) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (!s) {
    spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d no session for IP %s",
	     __FILE__, __LINE__, sw_pretty_ip(ip));
    if (sw_session_new(&s, ip, &sp) == -1) {
      return -1;
    }
  }

  if (state) *state = s->state;
  if (session_id) *session_id = s->session_id;
  if (interim_interval) *interim_interval = s->interim_interval;
  if (session_context) *session_context = s->session_context;
  if (termination_cause) *termination_cause = s->termination_cause;
  if (filter_name && sw_filter_get_name((CONST char **)filter_name, /* XXX */
                                        s->filter_id) == -1) {
      spinlock_unlock(sp);
      return -1;
  }
  spinlock_unlock(sp);
  return 0;
}

int sw_session_status_nonblock(uint32_t ip,
                      int *state,
                      uint32_t *session_id,
                      char **filter_name,
                      time_t *interim_interval,
                      char **session_context,
                      int *termination_cause) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs, &sp);
  if (!s) {
    spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d no session for IP %s",
         __FILE__, __LINE__, sw_pretty_ip(ip));
    return -1;
  }

  if (state) *state = s->state;
  if (session_id) *session_id = s->session_id;
  if (interim_interval) *interim_interval = s->interim_interval;
  if (session_context) *session_context = s->session_context;
  if (termination_cause) *termination_cause = s->termination_cause;
  if (filter_name && sw_filter_get_name((CONST char **)filter_name, /* XXX */
                                        s->filter_id) == -1) {
      spinlock_unlock(sp);
      return -1;
  }
  spinlock_unlock(sp);
  return 0;
}

int sw_session_verify_stateid(uint32_t ip, int *stateid) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (!s) {
     spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d no session for IP %s",
         __FILE__, __LINE__, sw_pretty_ip(ip));
    return 0;
  }
  if (*stateid && s->stateid != *stateid) {
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d mismatched stateid: #%d vs #%d", __FILE__, __LINE__, 
             *stateid, s->stateid);
    *stateid = s->stateid;
    spinlock_unlock(sp);
    return -1;
  }
  spinlock_unlock(sp);
  return 0; 
}

int sw_session_acquire_stateid(uint32_t ip, int *stateid, int modified) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (!s) {
      sw_log("%s:%d no session for IP %s",
         __FILE__, __LINE__, sw_pretty_ip(ip));
    spinlock_unlock(sp);
    return -1;
  }
  if (modified) {
    if (s->stateid == 0x7fffffff) {
      s->stateid = 0;
    }
    s->stateid++;
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d stateid incremented: #%d",
             __FILE__, __LINE__, s->stateid);
  }
  *stateid = s->stateid;
  spinlock_unlock(sp);
  return 0;
}

int sw_session_get_filter_id(uint32_t ip) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp = NULL;
  int ret = 0;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (!s) {
     spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d no session for IP %s",
	     __FILE__, __LINE__, sw_pretty_ip(ip));
    if (sw_session_new(&s, ip,&sp) == -1) {
      return -1;
    }
    spinlock_unlock(sp);
    return -1;
  }
  ret = s->filter_id;
  spinlock_unlock(sp);
  return ret;
}

int sw_session_retention(uint32_t ip, time_t *retention) {
  sw_session_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp = NULL;
  s = (sw_session_entry_t *)lh_retrieve(global_session_hash, (void *)&qs,&sp);
  if (!s) {
      spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_SESSION)
      sw_log("%s:%d no session for IP %s",
         __FILE__, __LINE__, sw_pretty_ip(ip));
    return -1;
  }
  if (retention) {
    if (*retention > 0) {
      if (s->deathday) {
        s->deathday += *retention - s->retention;
        if (sw_debug_flags & SW_DEBUG_SESSION)
          sw_log("%s:%d will expire session %s in %ld secs",
                 __FILE__, __LINE__, sw_pretty_ip(ip), s->deathday-time(NULL));
      }
      if (sw_debug_flags & SW_DEBUG_SESSION)
        sw_log("%s:%d retention time modified from %d to %d for IP %s",
               __FILE__, __LINE__, s->retention, *retention, sw_pretty_ip(ip));
      s->retention = *retention;
    }
    *retention = s->retention;
  }
  spinlock_unlock(sp);
  return 0;
}

int sw_session_expire() {
  time_t now = time(NULL);
  unsigned long down_load = global_session_hash->down_load;
  global_session_hash->down_load=0;
  lh_doall_arg(global_session_hash, __expire_session_cb, (char *)&now,0);
  global_session_hash->down_load = down_load;
  return 0;
}
