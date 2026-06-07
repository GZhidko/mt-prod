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
#include "nat_dst.h"
#include "nat_tuple.h"
#include "cfg.h"
#include "log.h"
#include <pthread.h>
#include "mTable.h"
#include "nat_src.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* Here we have all relations hashed by their two keys separately */
static LHASH *global_nat_dst_dst_hash=(LHASH*)0;
static LHASH *global_nat_dst_nat_hash=(LHASH*)0;
static LHASH *global_nat_dst_seq_hash=(LHASH*)0;
static uint32_t global_seq_ticker = 0;
static spinlock sp_gst_dst = SPINLOCK_INIT;
//static pthread_rwlock_t nat_dst_lock_rw = PTHREAD_RWLOCK_INITIALIZER;




static unsigned long __dst_hash_cb(sw_nat_dst_relation_t *r) {
  return sw_nat_tuple_hash(r->src) ^ \
    sw_nat_tuple_hash(r->dst) ^ \
    sw_nat_tuple_hash(r->nat);
}

static int __dst_cmp_cb(sw_nat_dst_relation_t *r1, sw_nat_dst_relation_t *r2) {
  return sw_nat_tuple_cmp(r1->src, r2->src) || \
    sw_nat_tuple_cmp(r1->dst, r2->dst) || \
    sw_nat_tuple_cmp(r1->nat, r2->nat);
}

static unsigned long __nat_hash_cb(sw_nat_dst_relation_t *r) {
  return sw_nat_tuple_hash(r->src) ^ sw_nat_tuple_hash(r->nat);
}

static int __nat_cmp_cb(sw_nat_dst_relation_t *r1, sw_nat_dst_relation_t *r2) {
  return sw_nat_tuple_cmp(r1->src, r2->src) || \
    sw_nat_tuple_cmp(r1->nat, r2->nat);
}

static unsigned long __seq_hash_cb(sw_nat_dst_relation_t *r) {
  return r->seq;
}

static int __seq_cmp_cb(sw_nat_dst_relation_t *r1, sw_nat_dst_relation_t *r2) {
  return r1->seq != r2->seq;
}

int sw_nat_dst_create() {
  global_nat_dst_dst_hash = lh_new(__dst_hash_cb, __dst_cmp_cb,37461120);
  if (!global_nat_dst_dst_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  global_nat_dst_nat_hash = lh_new(__nat_hash_cb, __nat_cmp_cb,37461120);
  if (!global_nat_dst_nat_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  global_nat_dst_seq_hash = lh_new(__seq_hash_cb, __seq_cmp_cb,37461120);
  if (!global_nat_dst_seq_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  sw_lhstat_new("dnat-dst", global_nat_dst_dst_hash);
  sw_lhstat_new("dnat-nat", global_nat_dst_nat_hash);
  sw_lhstat_new("dnat-seq", global_nat_dst_seq_hash);

  return 0;
}

int sw_nat_dst_do(sw_tuple_t *src, sw_tuple_t *dst, sw_tuple_t *nat) {
  sw_nat_dst_relation_t *r, qr;

  spinlock * sp_dd = NULL;
  spinlock  *sp_dn = NULL;
  spinlock  *sp_ds = NULL;

  if (!global_nat_dst_dst_hash || !global_nat_dst_nat_hash || \
      !global_nat_dst_seq_hash)
    return -1;

  qr.src = src; qr.dst = dst; qr.nat = nat;
  //pthread_rwlock_wrlock(&nat_dst_lock_rw);
  r = (sw_nat_dst_relation_t *)lh_retrieve(global_nat_dst_dst_hash, (void *)&qr,&sp_dd);
  if (r) {
      if (sw_debug_flags & SW_DEBUG_NAT_DST)
        sw_log("%s:%d existing relation #%u %s->%s [%s]", __FILE__, __LINE__,
               r->seq, sw_tuple_pretty(r->src), sw_tuple_pretty(r->dst),
               sw_tuple_pretty(r->nat));
      spinlock_unlock(sp_dd);
  } else {
    spinlock_unlock(sp_dd);
    spinlock_lock(&sp_gst_dst);
    qr.seq = global_seq_ticker++;
    if (global_seq_ticker >= SW_NAT_DST_RELATIONS_MAX)
      global_seq_ticker = 0;
    spinlock_unlock(&sp_gst_dst);
    r = (sw_nat_dst_relation_t *)lh_retrieve(global_nat_dst_seq_hash, (void *)&qr,&sp_ds);
    if (r) {
      //pthread_mutex_lock(&r->mtx);
      if (sw_debug_flags & (SW_DEBUG_NAT_DST | SW_DEBUG_RELATIONS))
        sw_log("%s:%d expiring relation #%u [%s] %s->%s", __FILE__, __LINE__,
               r->seq, sw_tuple_pretty(r->dst), sw_tuple_pretty(r->nat),
               sw_tuple_pretty(r->src));
      lh_delete(global_nat_dst_dst_hash, (void *)r,&sp_dd);
      lh_delete(global_nat_dst_nat_hash, (void *)r,&sp_dn);
      lh_delete(global_nat_dst_seq_hash, (void *)r,NULL);
      sw_tuple_free(r->src);
      sw_tuple_free(r->dst);
      sw_tuple_free(r->nat);


      //pthread_mutex_unlock(&r->mtx);
      free(r);
      spinlock_unlock(sp_dn);
      spinlock_unlock(sp_dd);
    }
     spinlock_unlock(sp_ds);
    r = (sw_nat_dst_relation_t *)malloc(sizeof(sw_nat_dst_relation_t));
    //pthread_mutexattr_init(&r->Attr);
    //pthread_mutexattr_settype(&r->Attr,PTHREAD_MUTEX_RECURSIVE);
    //pthread_mutex_init(&r->mtx,NULL);
    //specMutexLock(&r->mtx);
    if (!r) {
      sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
      //pthread_rwlock_unlock(&nat_dst_lock_rw);

      return -1;
    }
    if (sw_tuple_clone(&r->src, src) == -1 ||
        sw_tuple_clone(&r->dst, dst) == -1 ||
        sw_tuple_clone(&r->nat, nat) == -1) {
      sw_tuple_free(r->src);
      sw_tuple_free(r->dst);
      sw_tuple_free(r->nat);
      free(r);
      //pthread_rwlock_unlock(&nat_dst_lock_rw);
      return -1;
    }
    //spinlock_lock(&sp_gst_dst);
    r->seq = qr.seq;
    //spinlock_unlock(&sp_gst_dst);
    lh_insert(global_nat_dst_seq_hash, (void *)r,&sp_ds);
    lh_insert(global_nat_dst_dst_hash, (void *)r,&sp_dd);
    lh_insert(global_nat_dst_nat_hash, (void *)r,&sp_dn);

    if (sw_debug_flags & SW_DEBUG_NAT_DST)
      sw_log("%s:%d new relation #%u %s->%s [%s]", __FILE__, __LINE__,
             r->seq, sw_tuple_pretty(src), sw_tuple_pretty(dst), 
             sw_tuple_pretty(nat));
    //pthread_rwlock_unlock(&nat_dst_lock_rw);


    spinlock_unlock(sp_dn);
    spinlock_unlock(sp_dd);
    spinlock_unlock(sp_ds);
    return 0;
  }
  //pthread_rwlock_unlock(&nat_dst_lock_rw);
  return 0;
}

int sw_nat_dst_undo(sw_tuple_t **src, sw_tuple_t *nat, sw_tuple_t *dst) {
  sw_nat_dst_relation_t *r, qr;
  spinlock * sp = NULL;

  if (!global_nat_dst_dst_hash || !global_nat_dst_nat_hash || \
      !global_nat_dst_seq_hash)
    return -1;

  qr.src = dst; qr.dst = NULL; qr.nat = nat;
  //pthread_rwlock_wrlock(&nat_dst_lock_rw);
  r = (sw_nat_dst_relation_t *)lh_retrieve(global_nat_dst_nat_hash, (void *)&qr,&sp);
  if (r) {
      //pthread_mutex_lock(&r->mtx);
      if (sw_debug_flags & SW_DEBUG_NAT_DST)
        sw_log("%s:%d existing relation #%u [%s] %s->%s", __FILE__, __LINE__,
               r->seq, sw_tuple_pretty(r->dst), sw_tuple_pretty(r->nat),
               sw_tuple_pretty(r->src));

      //sw_tuple_free(*src);
      *src = NULL;
      if (sw_tuple_clone(src,r->dst) == -1) {
        sw_tuple_free(*src);
        *src = NULL;
        spinlock_unlock(sp);
        //pthread_rwlock_unlock(&nat_src_lock_rw);
        return -1;
      }
      //*src = r->dst;
      spinlock_unlock(sp);
      //pthread_mutex_unlock(&r->mtx);
      //pthread_rwlock_unlock(&nat_dst_lock_rw);
      return 0;
  } else {
    if (sw_debug_flags & SW_DEBUG_NAT_DST)
      sw_log("%s:%d no relation for %s->%s", __FILE__, __LINE__,
         sw_tuple_pretty(nat), sw_tuple_pretty(dst));
    if(*src)sw_tuple_free(*src);
    *src = NULL;
    //pthread_rwlock_unlock(&nat_dst_lock_rw);
    spinlock_unlock(sp);
    return -1;
  }
}
