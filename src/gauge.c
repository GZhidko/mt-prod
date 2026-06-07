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
#include "pretty.h"
#include "cfg.h"
#include "gauge.h"
#include "gauge_index.h"
#include "tuple.h"
#include "session.h"
#include "lhstat.h"
#include "log.h"
#include "mutexCirularBuffer.h"
#include "tuple.h"
#include "sweetspot.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static LHASH *global_gauge_hash=(LHASH*)0;

static int global_gauge_rate_interval;
static pthread_t threads[NUM_GAUGE_THREAD];


typedef struct __bufer_list_t__{
    CircularBufferGauge * val;
    struct __bufer_list_t__ * next;
}bufer_list_t;

static bufer_list_t buf_list = {NULL,NULL};

void gauge_buffer_register(CircularBufferGauge *buf) {
    bufer_list_t *i = (bufer_list_t *)&buf_list;
    while (i->next != NULL) {
        i = i->next;
    }
    if (i->val == NULL) {
        i->val = buf;
    } else {
        bufer_list_t *node = malloc(sizeof(bufer_list_t));
        if (node) {
            node->val = buf;
            node->next = NULL; // У��анавливаем �каза�ел� на �лед���ий �зел как NULL
            i->next = node;
        } else {
            // �б�або�ка о�ибки в�делени� пам��и дл� нового �зла
        }
    }
}

void *run_gauge_thread() {
    while (global_alive_flag) {
        bufer_list_t *i = (bufer_list_t *)&buf_list;
        while (i->val != NULL) {
            CircularBufferGauge *cb = i->val;
            gauge_tuple_t *gauge_tpl = dequeue(cb);
            if (gauge_tpl != NULL) {
                sw_gauge_update(gauge_tpl->chain, gauge_tpl->octets, gauge_tpl->direction);
                sw_tuple_free(gauge_tpl->chain);
                free(gauge_tpl);
            }
            if (!i->next) break;
            i = i->next;
        }
    }
    return NULL;
}

static unsigned long __hash_cb(sw_gauge_entry_t *e) {
  return e->ip;
}

static int __cmp_cb(sw_gauge_entry_t *e1, sw_gauge_entry_t *e2) {
  return e1->ip != e2->ip;
}

static void __expire_gauge_cb(sw_gauge_entry_t *s, time_t *now) {
  /* Throughput */
  time_t interval = *now - s->bps_probe_time;
  if (interval > global_gauge_rate_interval) {
    s->bps_in = s->bps_probe_in/interval;
    if (s->bps_in > s->peak_bps_in) {
      if (sw_debug_flags & (SW_DEBUG_GAUGE | SW_DEBUG_ACCT))
        sw_log("%s:%d new max inbound rate %llu bps for %s",
               __FILE__, __LINE__, s->bps_in, sw_pretty_ip(s->ip));
      s->peak_bps_in = s->bps_in;
    }
    s->bps_out = s->bps_probe_out/interval;
    if (s->bps_out > s->peak_bps_out) {
      if (sw_debug_flags & (SW_DEBUG_GAUGE | SW_DEBUG_ACCT))
        sw_log("%s:%d new max outbound rate %llu bps for %s",
               __FILE__, __LINE__, s->bps_out, sw_pretty_ip(s->ip));
      s->peak_bps_out = s->bps_out;
    }
    s->bps_probe_in = s->bps_probe_out = 0;
    s->bps_probe_time = *now;
  }
  /* Expire */
  if ((s->deathday && s->deathday < *now) ||
      s->max_idle && s->idle + s->max_idle < *now) {
    if (sw_debug_flags & (SW_DEBUG_GAUGE | SW_DEBUG_BREATH))
      sw_log("%s:%d expiring %s gauge %s", __FILE__, __LINE__, 
             s->max_idle && s->idle + s->max_idle < *now ? "idle" : "live",
             sw_pretty_ip(s->ip));
    sw_session_stop(s->ip, NULL,
                    s->max_idle && s->idle + s->max_idle < *now ? 
                    SW_SESSION_TERM_IDLE : SW_SESSION_TERM_TIME, "", "");
    sw_gauge_del(s->ip,0);
  }
}

int sw_gauge_create() {
  global_gauge_hash = lh_new(__hash_cb, __cmp_cb,(sw_netset_size(global_ns)+10)*2);
  if (!global_gauge_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  sw_lhstat_new("gauge", global_gauge_hash);

  global_gauge_rate_interval = atoi(sw_config_get("gauge-rate-interval", SW_GAUGE_RATE_INTERVAL));

  for (int i = 0; i < NUM_GAUGE_THREAD; ++i) {
      if (pthread_create(&threads[i], NULL, run_gauge_thread, NULL) != 0) {
          sw_log("%s:%d gauge thread for nat_src hash failed: %s", __FILE__, __LINE__, strerror(errno));
          return -1;
      }
      usleep(100);
  }

  return 0;
}

int sw_gauge_destroy() {
  if (sw_debug_flags & SW_DEBUG_GAUGE) 
    sw_log("%s:%d destroying gauge hash", __FILE__, __LINE__);

  sw_lhstat_del(global_gauge_hash);

  lh_free(global_gauge_hash);

  return 0;
}

int sw_gauge_new(uint32_t ip,
		 uint64_t max_octets_in,
		 uint64_t max_octets_out,
		 uint64_t max_bps_in,
		 uint64_t max_bps_out,
		 time_t max_duration,
		 time_t max_idle) {
  sw_gauge_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp =NULL;
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, &sp);
  if (!s) {
     spinlock_unlock(sp);
    s = malloc(sizeof(sw_gauge_entry_t));
    if (s == NULL) {
      sw_log("%s:%d malloc() failed for %s", __FILE__, __LINE__,
             sw_pretty_ip(ip));
      return -1;
    }
    memset(s, 0, sizeof(sw_gauge_entry_t));
    s->ip = ip;
    s->deathday = s->idle = s->bps_probe_time = time(NULL);
    lh_insert(global_gauge_hash, (void *)s,&sp);

    if (sw_debug_flags & SW_DEBUG_GAUGE) 
      sw_log("%s:%d gauge for %s created", 
        __FILE__, __LINE__, sw_pretty_ip(ip));
  }
  s->max_octets_in = max_octets_in;
  s->max_octets_out = max_octets_out;
  s->max_bps_in = max_bps_in;
  s->max_bps_out = max_bps_out;
  s->deathday += max_duration - s->max_duration;
  s->max_duration = max_duration;
  s->max_idle = max_idle;

  if (sw_debug_flags & SW_DEBUG_GAUGE) 
    sw_log("%s:%d gauge for %s updated", 
      __FILE__, __LINE__, sw_pretty_ip(ip));
  spinlock_unlock(sp);
  return 0;
}

int sw_gauge_del(uint32_t ip,uint8_t blocking) {
  sw_gauge_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp =NULL;
  if(blocking){
    s = (sw_gauge_entry_t *)lh_delete(global_gauge_hash, (void *)&qs,&sp);
    spinlock_unlock(sp);
  }
  else
  {
    s = (sw_gauge_entry_t *)lh_delete(global_gauge_hash, (void *)&qs,NULL);
  }
  if (s) {
    free(s);
  }
  return 0;
}


int sw_gauge_drop_stat(CircularBufferGauge * cb, sw_tuple_t *tuple_chain, uint64_t octets, int direction) {
  gauge_tuple_t * gauge_tpl;
  //return 0;
    if (!NUM_GAUGE_THREAD){
      sw_gauge_update(tuple_chain, octets, direction);
      return 0;
  }   
  gauge_tpl = malloc(sizeof(gauge_tuple_t));
  if(!gauge_tpl)
  {
      sw_log("%s:%d malloc failed",__FILE__, __LINE__);
      return -1;
  }
  sw_tuple_clone(&gauge_tpl->chain,tuple_chain);
  if(!gauge_tpl->chain)
  {
      sw_log("%s:%d tuple clone failed tuple:%s",__FILE__, __LINE__, sw_tuple_pretty(tuple_chain));
      return -1;
  }
  gauge_tpl->direction = direction;
  gauge_tpl->octets = octets;
  if(!enqueue(cb, gauge_tpl))
  {
      sw_log("%s:%d circular buffer is full",
             __FILE__, __LINE__);
      return -1;
  }

  return 0;
}

int sw_gauge_update(sw_tuple_t *tuple_chain, uint64_t octets, int direction) {
  sw_gauge_entry_t qs, *s;
  uint32_t ip;
  spinlock * sp =NULL;

  if (sw_gauge_index(&ip, tuple_chain) == -1) {
    return -1;
  }

  qs.ip = ip;
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, &sp);
  if (!s) {
      spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_GAUGE)
      sw_log("%s:%d no gauge entry for IP %s",
             __FILE__, __LINE__, sw_pretty_ip(ip));
    return 0;
  }

  /* Update traffic counters */
  switch(direction) {
  case SW_GAUGE_DIR_IN:
    s->octets_in += octets;
    s->bps_probe_in += octets * 8;
    s->packets_in++;
    break;
  case SW_GAUGE_DIR_OUT:
    s->octets_out += octets;
    s->bps_probe_out += octets * 8;
    s->packets_out++;
    break;
  }

  s->idle = time(NULL);

  /* Limit traffic */
  if (s->octets_in > s->max_octets_in || s->octets_out > s->max_octets_out) {
    if (sw_debug_flags & (SW_DEBUG_GAUGE | SW_DEBUG_BREATH))
      sw_log("%s:%d throttling %s, octets in %llu (max %llu), out %llu (max %llu)", __FILE__, __LINE__,  sw_pretty_ip(ip), s->octets_in, s->max_octets_in, s->octets_out, s->max_octets_out);
    //spinlock_unlock(sp);
    spinlock_unlock(sp);
    sw_session_stop(ip, NULL, SW_SESSION_TERM_VOLUME, "", "");
    sw_gauge_del(ip,1);

    return 0;
  }
  spinlock_unlock(sp);
  return 0;
}

int sw_gauge_limits(uint32_t ip,
                    uint64_t *max_octets_in, 
                    uint64_t *max_octets_out,
                    uint64_t *max_bps_in, 
                    uint64_t *max_bps_out,
                    time_t *max_duration, 
                    time_t *max_idle,
                    int block) {
  sw_gauge_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp =NULL;
  if(block)
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, &sp);
  else
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, NULL);
  if (!s) {
    spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_GAUGE)
      sw_log("%s:%d no gauge entry for IP %s",
             __FILE__, __LINE__, sw_pretty_ip(ip));
    return -1;
  }

  if (max_octets_in) *max_octets_in = s->max_octets_in;
  if (max_octets_out) *max_octets_out = s->max_octets_out;
  if (max_bps_in) *max_bps_in = s->max_bps_in;
  if (max_bps_out) *max_bps_out = s->max_bps_out;
  if (max_duration) *max_duration = s->max_duration;
  if (max_idle) *max_idle = s->max_idle;
  spinlock_unlock(sp);
  return 0;
}

int sw_gauge_current(uint32_t ip,
                     uint64_t *octets_in, 
                     uint64_t *octets_out,
                     uint64_t *packets_in, 
                     uint64_t *packets_out,
                     uint64_t *bps_in,
                     uint64_t *bps_out,
                     time_t *duration, 
                     time_t *idle,
                     int block) {
  sw_gauge_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp =NULL;
  if(block)
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, &sp);
  else
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, NULL);
  if (!s) {
      spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_GAUGE)
      sw_log("%s:%d no gauge entry for IP %s",
             __FILE__, __LINE__, sw_pretty_ip(ip));
    return -1;
  }

  if (octets_in) *octets_in = s->octets_in;
  if (octets_out) *octets_out = s->octets_out;
  if (packets_in) *packets_in = s->packets_in;
  if (packets_out) *packets_out = s->packets_out;
  if (bps_in) *bps_in = s->bps_in;
  if (bps_out) *bps_out = s->bps_out;
  if (duration) *duration = time(NULL) - (s->deathday - s->max_duration);
  if (idle) *idle = time(NULL) - s->idle;
  spinlock_unlock(sp);
  return 0;
}

int sw_gauge_peak(uint32_t ip,
                  uint64_t *bps_in,
                  uint64_t *bps_out,
                  int block) {
  sw_gauge_entry_t qs, *s;
  qs.ip = ip;
  spinlock * sp =NULL;
  if(block)
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, &sp);
  else
  s = (sw_gauge_entry_t *)lh_retrieve(global_gauge_hash, (void *)&qs, NULL);
  if (!s) {
      spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_GAUGE)
      sw_log("%s:%d no gauge entry for IP %s",
             __FILE__, __LINE__, sw_pretty_ip(ip));
    return -1;
  }

  if (bps_in) *bps_in = s->peak_bps_in;
  if (bps_out) *bps_out = s->peak_bps_out;
spinlock_unlock(sp);
  return 0;
}

int sw_gauge_expire() {
  time_t now = time(NULL);
  unsigned long down_load = global_gauge_hash->down_load;
  global_gauge_hash->down_load=0;
  lh_doall_arg(global_gauge_hash, __expire_gauge_cb, (char *)&now, 0);
  global_gauge_hash->down_load = down_load;
  return 0;
}
