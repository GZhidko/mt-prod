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
#include "nat_src.h"
#include "nat_src_endpoint.h"
#include "nat_tuple.h"
#include "lhstat.h"
#include "cfg.h"
#include "log.h"
#include <pthread.h>
#include "mTable.h"
#include "sweetspot.h"
 #include <unistd.h>
#include "netset.h"
#include "session.h"
#include "nat_src_endpoint_ip.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

int sw_nat_src_enabled_flag = 0;



/* Here we have all relations hashed by their two keys separately */
static LHASH *global_nat_src_prv_hash=(LHASH*)0;
static LHASH *global_nat_src_pub_hash=(LHASH*)0;
/* ...and have each relation scheduled for eventual removal */
//static LHASH *global_nat_src_seq_hash=(LHASH*)0;
//static pthread_rwlock_t nat_src_lock_rw = PTHREAD_RWLOCK_INITIALIZER;

static uint32_t global_seq_ticker = 0;
spinlock sp_gst = SPINLOCK_INIT;
static pthread_t threads[NUM_GARBAGE_THREAD];


static int sw_nat_src_rel_expire();

static unsigned long __prv_hash_cb(sw_nat_src_relation_t *r) {
  return sw_nat_tuple_hash(r->prv);
}

static int __prv_cmp_cb(sw_nat_src_relation_t *r1, sw_nat_src_relation_t *r2) {
  return sw_nat_tuple_cmp(r1->prv, r2->prv);
}

static unsigned long __pub_hash_cb(sw_nat_src_relation_t *r) {
  return sw_nat_tuple_hash(r->pub);
}

static int __pub_cmp_cb(sw_nat_src_relation_t *r1, sw_nat_src_relation_t *r2) {
  return sw_nat_tuple_cmp(r1->pub, r2->pub);
}

static unsigned long __seq_hash_cb(sw_nat_src_relation_t *r) {
  return r->seq;
}

static int __seq_cmp_cb(sw_nat_src_relation_t *r1, sw_nat_src_relation_t *r2) {
  return r1->seq != r2->seq;
}





static int sw_nat_src_relation_clone(sw_nat_src_relation_t **r1, sw_nat_src_relation_t *r2)
{
  if(!r2)
      return -1;
  (*r1) = (sw_nat_src_relation_t*) malloc(sizeof(sw_nat_src_relation_t));
  if (!*r1)
  {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  };
  //memcpy(&(*r1),r2,sizeof(sw_nat_src_relation_t));
  (*r1)->pub = NULL;
  (*r1)->prv = NULL;
  if (sw_tuple_clone(&(*r1)->pub,r2->pub) == -1)
  {
      sw_tuple_free((*r1)->pub);
      (*r1)->pub = NULL;
      free(*r1);
      sw_log("%s:%d nat relation clone() failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
  }
  if (sw_tuple_clone(&(*r1)->prv,r2->prv) == -1)
  {
      sw_tuple_free((*r1)->pub);
      sw_tuple_free((*r1)->prv);
      (*r1)->prv = NULL;
      (*r1)->pub = NULL;
      free(*r1);
      sw_log("%s:%d nat relation clone() failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
  }
  (*r1)->seq = r2->seq;

  return 0;

};

static int sw_nat_src_relation_free(sw_nat_src_relation_t **r1)
{
  if(!*r1)
  {
      return -1;
      sw_log("%s:%d nat relation free() failed: %s", __FILE__, __LINE__, strerror(errno));
  }
      sw_tuple_free((*r1)->pub);
      sw_tuple_free((*r1)->prv);
      (*r1)->prv = NULL;
      (*r1)->pub = NULL;
      free(*r1);
  return 0;

};

void *run_garbage() {

  while (global_alive_flag) {
    usleep(10000000);
    sw_nat_src_rel_expire();
  }
    return 0;
};


int sw_nat_src_create() {

  global_nat_src_prv_hash = lh_new(__prv_hash_cb, __prv_cmp_cb,37461120);
  if (!global_nat_src_prv_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  global_nat_src_pub_hash = lh_new(__pub_hash_cb, __pub_cmp_cb,37461120);
  if (!global_nat_src_pub_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }


  sw_lhstat_new("snat-prv", global_nat_src_prv_hash);
  sw_lhstat_new("snat-pub", global_nat_src_pub_hash);


  // Create and run garbage threads
  // Set the number of threads you want


  for (int i = 0; i < NUM_GARBAGE_THREAD; ++i) {
      if (pthread_create(&threads[i], NULL, run_garbage, NULL) != 0) {
          sw_log("%s:%d garbage thread for nat_src hash failed: %s", __FILE__, __LINE__, strerror(errno));
          return -1;
      }
      usleep(100);
  }


  return 0;
}

void __expire_current_seq(sw_nat_src_relation_t * ri) {
  sw_nat_src_relation_t *r, qr;
  spinlock * sp_sp = NULL;
  r = NULL;
  time_t now = time(NULL);
  if (now - ri->last_dl_time > RELATION_EXPIRE_INTERVAL)
  {

        if(ri)
        {
        //if(pthread_mutex_lock(&r->mtx)!=0) return;
        if(ri->prv)
        lh_delete(global_nat_src_prv_hash, (void *)ri,&sp_sp);
        if(ri->pub)
        lh_delete(global_nat_src_pub_hash, (void *)ri,NULL);
            if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
                sw_log("%s:%d expiring relation  %s [%s]", __FILE__, __LINE__,
                  sw_tuple_pretty(ri->prv), sw_tuple_pretty(ri->pub));
            if(ri->prv)sw_tuple_free(ri->prv);if(ri->pub) sw_tuple_free(ri->pub);
            ri->prv = NULL;
            ri->pub = NULL;
            free(ri);
         spinlock_unlock(sp_sp);
        }
        else
        {
            sw_log("%s:%d garbage cant remove relation its dosent exist in snat_prv_hash  %s [%s]", __FILE__, __LINE__,
              sw_tuple_pretty(ri->prv), sw_tuple_pretty(ri->pub));
        }
  }


}
/*
 * void __expire_current_seq(uint32_t * rel) {
  sw_nat_src_relation_t *r, qr;
  spinlock * sp_sp = NULL;
  spinlock  *sp_spu = NULL;
  spinlock  *sp_ss = NULL;

  sw_nat_src_relation_t *r_clone;
  spinlock_lock(&sp_gst);
  qr.seq = ++global_seq_ticker;
  if (global_seq_ticker >= SW_NAT_SRC_RELATIONS_MAX)
    global_seq_ticker = 0;
  *rel = qr.seq;

  //global_seq_ticker++;
  spinlock_unlock(&sp_gst);
  r = (sw_nat_src_relation_t *)lh_retrieve(global_nat_src_seq_hash, (void *)&qr,&sp_ss);
  if (r) {
      if(sw_nat_src_relation_clone(&r_clone,r) == 0)
      {
        spinlock_unlock(sp_ss);
        //if(pthread_mutex_lock(&r->mtx)!=0) return;
        if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
          sw_log("%s:%d expiring relation #%u %s [%s]", __FILE__, __LINE__,
                 r_clone->seq, sw_tuple_pretty(r_clone->prv), sw_tuple_pretty(r_clone->pub));
        r = (sw_nat_src_relation_t *) lh_delete(global_nat_src_prv_hash, (void *)r_clone,&sp_sp);
        lh_delete(global_nat_src_pub_hash, (void *)r_clone,&sp_spu);
        lh_delete(global_nat_src_seq_hash, (void *)r_clone,&sp_ss);
        if(r)
        {

            //pthread_mutex_unlock(&r->mtx);
            sw_log("%s:%d remove relation #%u %s [%s]", __FILE__, __LINE__,
                   r->seq, sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));
            sw_tuple_free(r->prv); sw_tuple_free(r->pub);
            r->prv = NULL;
            r->pub = NULL;
            free(r);

        }
        else
           sw_log("%s:%d nat relation free() failed", __FILE__, __LINE__);

        spinlock_unlock(sp_ss);
        spinlock_unlock(sp_spu);
        spinlock_unlock(sp_sp);
        sw_nat_src_relation_free(&r_clone);
      }
  }
  else
  spinlock_unlock(sp_ss);
}*/

int sw_nat_src_do(sw_tuple_t **pub, sw_tuple_t *prv) {
  sw_nat_src_relation_t *r, qr, *ri;
  spinlock * sp_sp = NULL;
  spinlock  *sp_spu = NULL;
  spinlock  *sp_ss = NULL;
  int attempts = 0;
  int reused_preempted_relation = 0;
  time_t now = 0;

  sw_nat_src_relation_t * r_clone;





  if (!global_nat_src_prv_hash || !global_nat_src_pub_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }


  qr.prv = prv; qr.pub = NULL;
  //pthread_rwlock_wrlock(&nat_src_lock_rw);
  r = (sw_nat_src_relation_t *)lh_retrieve(global_nat_src_prv_hash, (void *)&qr,&sp_sp);
  if (r) {
      //if (sw_debug_flags & SW_DEBUG_NAT_SRC)
        if (sw_debug_flags & SW_DEBUG_NAT_SRC)
            sw_log("%s:%d existing relation %s to [%s] ", __FILE__, __LINE__,
                   sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));
      r->last_dl_time = time(NULL);

    if (sw_tuple_clone(pub,  r->pub) == -1) {
      sw_tuple_free(*pub);
      *pub = NULL;
      spinlock_unlock(sp_sp);
      //pthread_rwlock_unlock(&nat_src_lock_rw);
      return -1;
    }
    spinlock_unlock(sp_sp);
  } else {
    spinlock_unlock(sp_sp);
    r = NULL;
    *pub = NULL;

    for (attempts = 0; attempts < ASSIGN_RETRY_MAX; attempts++) {
      if (sw_tuple_clone(pub, prv) == -1) {
        sw_tuple_free(*pub);
        *pub = NULL;
        return -1;
      }

      if (sw_nat_src_endpoint_assign(*pub, prv, 0) == -1) {
        sw_tuple_free(*pub);
        *pub = NULL;
        return -1;
      }

      qr.pub = *pub; qr.prv = NULL;
      r = (sw_nat_src_relation_t *)lh_retrieve(global_nat_src_pub_hash, (void *)&qr,&sp_spu);

      if (!r) {
        spinlock_unlock(sp_spu);
        break;
      }

      now = time(NULL);
      if ((now - r->last_dl_time) < ACTIVE_REL_PROTECT_SEC) {
        if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS)) {
          sw_log("%s:%d skip preempt active relation %s [%s] age=%ld",
                 __FILE__, __LINE__,
                 sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub),
                 (long)(now - r->last_dl_time));
        }
        spinlock_unlock(sp_spu);
        sw_tuple_free(*pub);
        *pub = NULL;
        continue;
      }

      if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
        sw_log("%s:%d preempting relation %s [%s]", __FILE__, __LINE__,
                sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));
      if(r->prv)
      lh_delete(global_nat_src_prv_hash, (void *)r,&sp_sp);
      if(r->pub)
      lh_delete(global_nat_src_pub_hash, (void *)r,NULL);
      if(r)
      {
        if(r->prv)sw_tuple_free(r->prv); if(r->pub)sw_tuple_free(r->pub);
        r->pub = NULL;
        r->prv = NULL;
      }
      spinlock_unlock(sp_spu);
      spinlock_unlock(sp_sp);
      
      reused_preempted_relation = 1;
      break;
    }

    if (attempts == ASSIGN_RETRY_MAX) {
//      sw_log("%s:%d no free SNAT port without preempting active relation", __FILE__, __LINE__);
      sw_tuple_free(*pub);
      *pub = NULL;
      return -1;
    }

    if (!reused_preempted_relation) {
      r = (sw_nat_src_relation_t *)malloc(sizeof(sw_nat_src_relation_t));

      if (!r) {
        sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
        sw_tuple_free(*pub);
        *pub = NULL;
        return -1;
      }
    }
    if (sw_tuple_clone(&r->prv, prv) == -1) {
      //sw_tuple_free(*pub);
      sw_tuple_free(r->prv);
      *pub = NULL;
      if (!reused_preempted_relation) {
        free(r);
      }
      return -1;
    }

    if (sw_tuple_clone(&r->pub,*pub) == -1) {
      //sw_tuple_free(*pub);
      *pub = NULL;
      sw_tuple_free(r->prv);
      r->prv = NULL;
      if (!reused_preempted_relation) {
        free(r);
      }
      return -1;
    }
    //r->pub = *pub;

    r->last_dl_time = time(NULL);

lh_insert(global_nat_src_pub_hash, (void *)r,&sp_spu);
    lh_insert(global_nat_src_prv_hash, (void *)r,&sp_sp);





    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d new relation #%u %s [%s]", __FILE__, __LINE__,
             r->seq, sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));



    spinlock_unlock(sp_spu);
    spinlock_unlock(sp_sp);


  }

  //pthread_rwlock_unlock(&nat_src_lock_rw);
  return 0;
}

/*int sw_nat_src_do(sw_tuple_t **pub, sw_tuple_t *prv) {
  sw_nat_src_relation_t *r, qr;
  spinlock * sp_sp = NULL;
  spinlock  *sp_spu = NULL;
  spinlock  *sp_ss = NULL;

  if (!global_nat_src_prv_hash || !global_nat_src_pub_hash || \
      !global_nat_src_seq_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }


  qr.prv = prv; qr.pub = NULL;
  //pthread_rwlock_wrlock(&nat_src_lock_rw);
  r = (sw_nat_src_relation_t *)lh_retrieve(global_nat_src_prv_hash, (void *)&qr,&sp_sp);
  if (r) {
    //specMutexLock(&r->mtx);
    spinlock_lock(&sp_gst);
    uint32_t gst = global_seq_ticker;
    spinlock_unlock(&sp_gst);
    if (gst < r->seq &&
        r->seq - gst < SW_NAT_SRC_RELATIONS_AGE_MIN ||
        gst > r->seq &&
        SW_NAT_SRC_RELATIONS_MAX - gst + r->seq < SW_NAT_SRC_RELATIONS_AGE_MIN) {
      //if (sw_debug_flags & SW_DEBUG_NAT_SRC)
        sw_log("%s:%d refreshing relation #%u %s [%s] to #%u",
               __FILE__, __LINE__, r->seq, sw_tuple_pretty(r->prv),
               sw_tuple_pretty(r->pub), gst);
      lh_delete(global_nat_src_seq_hash, (void *)r,&sp_ss);
      spinlock_unlock(sp_ss);
      spinlock_lock(&sp_gst);
      __expire_current_seq();
      r->seq = global_seq_ticker++;
      spinlock_unlock(&sp_gst);
      lh_insert(global_nat_src_seq_hash, (void *)r,&sp_ss);
    } else {
      if (sw_debug_flags & SW_DEBUG_NAT_SRC)
        sw_log("%s:%d existing relation #%u %s [%s]", __FILE__, __LINE__,
               r->seq, sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));
    }
    //*pub = r->pub;
    if (sw_tuple_clone(pub,  r->pub) == -1) {
      sw_tuple_free(*pub);
      *pub = NULL;
      spinlock_unlock(sp_ss);
      spinlock_unlock(sp_sp);

      //pthread_rwlock_unlock(&nat_src_lock_rw);
      return -1;
    }
    spinlock_unlock(sp_ss);
    spinlock_unlock(sp_sp);

  } else {
    spinlock_unlock(sp_sp);
    if (sw_tuple_clone(pub, prv) == -1) {
      sw_tuple_free(*pub);
      *pub = NULL;
      //pthread_rwlock_unlock(&nat_src_lock_rw);
      return -1;
    }

    if (sw_nat_src_endpoint_assign(*pub, prv, 0) == -1) {
      sw_tuple_free(*pub);
      *pub = NULL;
      //pthread_rwlock_unlock(&nat_src_lock_rw);
      return -1;
    }

    qr.pub = *pub; qr.prv = NULL;
    r = (sw_nat_src_relation_t *)lh_retrieve(global_nat_src_pub_hash, (void *)&qr,&sp_spu);
    if (r) {
      //specMutexLock(&r->mtx);
      if (sw_debug_flags & (SW_DEBUG_NAT_SRC | SW_DEBUG_RELATIONS))
        sw_log("%s:%d preempting relation #%u %s [%s]", __FILE__, __LINE__,
               r->seq, sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));
      lh_delete(global_nat_src_prv_hash, (void *)r,&sp_sp);
      lh_delete(global_nat_src_pub_hash, (void *)r,NULL);
      lh_delete(global_nat_src_seq_hash, (void *)r,&sp_ss);
      sw_tuple_free(r->prv); sw_tuple_free(r->pub);
      spinlock_unlock(sp_ss);
      spinlock_unlock(sp_sp);
    } else {
       spinlock_unlock(sp_spu);
      r = (sw_nat_src_relation_t *)malloc(sizeof(sw_nat_src_relation_t));
      //pthread_mutexattr_init(&r->Attr);
     // pthread_mutexattr_settype(&r->Attr,PTHREAD_MUTEX_RECURSIVE);
     // pthread_mutex_init(&r->mtx,NULL);
     // specMutexLock(&r->mtx);
      if (!r) {
        sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
        sw_tuple_free(*pub); free(r);
        *pub = NULL;
        //pthread_rwlock_unlock(&nat_src_lock_rw);
        return -1;
      }
    }
    if (sw_tuple_clone(&r->prv, prv) == -1) {
      sw_tuple_free(*pub); sw_tuple_free(r->prv); free(r);
      *pub = NULL;
      //pthread_rwlock_unlock(&nat_src_lock_rw);
      spinlock_unlock(sp_spu);
      return -1;
    }

    if (sw_tuple_clone(&r->pub,*pub) == -1) {
      sw_tuple_free(*pub);
      *pub = NULL;
      spinlock_unlock(sp_spu);
      //pthread_rwlock_unlock(&nat_src_lock_rw);
      return -1;
    }
    //r->pub = *pub;
    spinlock_lock(&sp_gst);
    __expire_current_seq();
    r->seq = global_seq_ticker++;
    spinlock_unlock(&sp_gst);
    //spinlock_unlock(sp_spu);

    lh_insert(global_nat_src_prv_hash, (void *)r,&sp_sp);
    lh_insert(global_nat_src_pub_hash, (void *)r,NULL);
    lh_insert(global_nat_src_seq_hash, (void *)r,&sp_ss);


    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d new relation #%u %s [%s]", __FILE__, __LINE__,
             r->seq, sw_tuple_pretty(r->prv), sw_tuple_pretty(r->pub));

    spinlock_unlock(sp_spu);
    spinlock_unlock(sp_sp);
    spinlock_unlock(sp_ss);
  }

  //pthread_rwlock_unlock(&nat_src_lock_rw);
  return 0;
}*/

int sw_nat_src_undo(sw_tuple_t **prv, sw_tuple_t *pub) {
  sw_nat_src_relation_t *r, qr;
  spinlock * sp = NULL;
  sw_nat_src_relation_t * r_clone;


  if (!global_nat_src_prv_hash || !global_nat_src_pub_hash) {
    sw_log("%s:%d SNAT not initialized", __FILE__, __LINE__);
    return -1;
  }

  qr.prv = NULL; qr.pub = pub;
  //pthread_rwlock_wrlock(&nat_src_lock_rw);
  r = (sw_nat_src_relation_t *)lh_retrieve(global_nat_src_pub_hash, (void *)&qr,&sp);
  if (r) {
    //specMutexLock(&r->mtx);
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)  
      sw_log("%s:%d existing relation  [%s] %s", __FILE__, __LINE__,
              sw_tuple_pretty(r->pub), sw_tuple_pretty(r->prv));
    if (sw_tuple_clone(prv,r->prv) == -1) {
      sw_tuple_free(*prv);
      *prv = NULL;
      spinlock_unlock(sp);
      sw_nat_src_relation_free(&r_clone);
      //pthread_rwlock_unlock(&nat_src_lock_rw);
      return -1;
    }
  } else {
    if (sw_debug_flags & SW_DEBUG_NAT_SRC)
      sw_log("%s:%d no relation for dst %s", __FILE__, __LINE__,
         sw_tuple_pretty(pub));
    spinlock_unlock(sp);
    //pthread_rwlock_unlock(&nat_src_lock_rw);
    return -1;
  }
  spinlock_unlock(sp);
  //pthread_rwlock_unlock(&nat_src_lock_rw);

  return 0;
}


int sw_nat_src_rel_expire() {

  unsigned long down_load = global_nat_src_pub_hash->down_load;
  //global_session_hash->down_load=0;
  lh_doall_arg(global_nat_src_pub_hash, __expire_current_seq, NULL,0);
  global_nat_src_pub_hash->down_load = down_load;
  //sw_log("%s:%d finish garbage cycle", __FILE__, __LINE__);
  return 0;
}


void sw_nat_src_stop_garbage()
{
    for (int i = 0; i < NUM_GARBAGE_THREAD; ++i) {
        if (pthread_cancel(threads[i]) != 0) {
            sw_log("%s:%d stop garbage thread for nat_src hash failed: %s", __FILE__, __LINE__, strerror(errno));
        }
    }
};
