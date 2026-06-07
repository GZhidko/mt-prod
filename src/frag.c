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
#include <errno.h>
#include "lhash.h"
#include "tuple.h"
#include "pretty.h"
#include "fnv.h"
#include "frag.h"
#include "frag_type.h"
#include "frag_tuple.h"
#include "lhstat.h"
#include "cfg.h"
#include "log.h"
#include <pthread.h>
#include <spinlock.h>
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Here we have all relations hashed by their two keys separately */
static LHASH *global_frag_iph_hash=(LHASH*)0;
static LHASH *global_frag_seq_hash=(LHASH*)0;
static uint32_t global_seq_ticker = 0;
//static pthread_rwlock_t lock_rw = PTHREAD_RWLOCK_INITIALIZER;

static unsigned long __iph_hash_cb(sw_frag_track_t *t) {
  /* It's important not to XOR here to distinguish packet direction */
  struct _h {
    uint32_t src;
    uint32_t dst;
  } h;
  memset(&h, 0, sizeof(h));
  h.src = sw_frag_tuple_hash(t->src);
  h.dst = sw_frag_tuple_hash(t->dst);
  return fnv_32a_buf((char *)&h, sizeof(h), FNV1_32A_INIT);
}

static int __iph_cmp_cb(sw_frag_track_t *t1, sw_frag_track_t *t2) {
  return sw_frag_tuple_cmp(t1->src, t2->src) || \
    sw_frag_tuple_cmp(t1->dst, t2->dst);
}

static unsigned long __seq_hash_cb(sw_frag_track_t *t) {
  return t->seq;
}

static int __seq_cmp_cb(sw_frag_track_t *t1, sw_frag_track_t *t2) {
  return t1->seq != t2->seq;
}

int sw_frag_create() {
  global_frag_iph_hash = lh_new(__iph_hash_cb, __iph_cmp_cb,0);
  if (!global_frag_iph_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  global_frag_seq_hash = lh_new(__seq_hash_cb, __seq_cmp_cb,0);
  if (!global_frag_seq_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  sw_lhstat_new("frag-iph", global_frag_iph_hash);
  sw_lhstat_new("frag-seq", global_frag_seq_hash);

  return 0;
}

int sw_frag_do(sw_tuple_t **hsrc, sw_tuple_t **hdst,
               sw_tuple_t *src, sw_tuple_t *dst) {
  sw_frag_track_t *t, *et, qt;
  uint32_t type;
  spinlock * sp;
  spinlock  *sp_ip;



  if (!global_frag_iph_hash)
    return -1;


  if (sw_frag_type(&type, src) == -1) {
    return -1;
  }
  //pthread_rwlock_wrlock(&lock_rw);
  switch(type) {
  case SW_FRAG_TYPE_NONE:
    *hsrc = *hdst = NULL;
    break;
  case SW_FRAG_TYPE_HEAD:
    t = (sw_frag_track_t *)malloc(sizeof(sw_frag_track_t));
    if (!t) {
      sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
          //pthread_rwlock_unlock(&lock_rw);
      return -1;
    }
    if (sw_tuple_clone(&t->src, src) == -1 ||
        sw_tuple_clone(&t->dst, dst) == -1) {
      sw_tuple_free(t->src);
      sw_tuple_free(t->dst);
      //pthread_rwlock_unlock(&lock_rw);
      return -1;
    }

    t->active = 1;

    t->seq = global_seq_ticker++;
    if (global_seq_ticker == SW_FRAG_TRACKS_MAX)
      global_seq_ticker = 0;


    et = (sw_frag_track_t *)lh_delete(global_frag_seq_hash, (void *)t, &sp);

    if (et) {
      if (sw_debug_flags & (SW_DEBUG_FRAG | SW_DEBUG_RELATIONS))
        sw_log("%s:%d expiring %sactive track #%d %s->%s", 
               __FILE__, __LINE__, et->active ? "" : "in", et->seq, 
               sw_tuple_pretty(et->dst), sw_tuple_pretty(et->src));
      lh_delete(global_frag_iph_hash, (void *)et, &sp_ip);
      sw_tuple_free(et->src);
      sw_tuple_free(et->dst);

      free(et);
      spinlock_unlock(sp_ip);
    }
    spinlock_unlock(sp);




    //pthread_mutex_init(&t->mtx,NULL);
    
    lh_insert(global_frag_iph_hash, (void *)t, &sp_ip);
    lh_insert(global_frag_seq_hash, (void *)t, &sp);

    *hsrc = *hdst = NULL;

    if (sw_debug_flags & SW_DEBUG_FRAG)
      sw_log("%s:%d new track #%d %s->%s", __FILE__, __LINE__,
             t->seq, sw_tuple_pretty(src), sw_tuple_pretty(dst));

    spinlock_unlock(sp);
    spinlock_unlock(sp_ip);

    break;
  case SW_FRAG_TYPE_TAIL:
  case SW_FRAG_TYPE_BODY:
    qt.src = src; qt.dst = dst;

    t = (sw_frag_track_t *)lh_retrieve(global_frag_iph_hash, (void *)&qt, &sp_ip);
    if (t && t->active) {
      //specMutexLock(&t->mtx);
      //*hsrc = t->src; *hdst = t->dst;
      if (sw_tuple_clone(hsrc,t->src) == -1 ||
            sw_tuple_clone(hdst,t->dst) == -1) {
          sw_tuple_free(*hsrc);
          sw_tuple_free(*hdst);
          //pthread_rwlock_unlock(&lock_rw);
          spinlock_unlock(sp_ip);
          return -1;
        }

      if (sw_debug_flags & SW_DEBUG_FRAG)
        sw_log("%s:%d existing track #%d %s->%s", __FILE__, __LINE__,
               t->seq, sw_tuple_pretty(t->src), sw_tuple_pretty(t->dst));

      if (type == SW_FRAG_TYPE_TAIL) {
        //pthread_rwlock_unlock(&lock_rw);
        //pthread_rwlock_wrlock(&lock_rw);
        t->active = 0;
        if (sw_debug_flags & SW_DEBUG_FRAG)
          sw_log("%s:%d deactivate track #%d %s->%s", __FILE__, __LINE__,
                 t->seq, sw_tuple_pretty(t->src), sw_tuple_pretty(t->dst));
      }
    spinlock_unlock(sp_ip);
    } else {
      *hsrc = *hdst = NULL;
      if (sw_debug_flags & SW_DEBUG_FRAG)
        sw_log("%s:%d no %strack for fragment %s->%s", 
               __FILE__, __LINE__, t ? "active " : "",
               sw_tuple_pretty(src), sw_tuple_pretty(dst));
      spinlock_unlock(sp_ip);
    }
    break;
  default:
    sw_log("%s:%d sw_frag_tuple_part() failure for %s",
           __FILE__, __LINE__, sw_tuple_pretty(src));  
    //pthread_rwlock_unlock(&lock_rw);
    return -1;
  }
  //pthread_rwlock_unlock(&lock_rw);
  return 0;
}
