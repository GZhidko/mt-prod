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
#include <pthread.h>
#include "debug.h"
#include "tuple.h"
#include "tuple_ip.h"
#include "lhash.h"
#include "shape.h"
#include "memgmt.h"
#include "log.h"
#include "pretty.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define DROP_SHAPE_QUEUE 0
#define DROP_SHAPE_NO_CONTEXT 1
#define DROP_SHAPE_TIMESLOT_FULL 2
#define DROP_SHAPE_QUEUE_SEND_FAILED 3
#define SHAPE_SCHEDULE 4
#define SHAPE_DELAY 5

static LHASH *global_shape_hash=(LHASH*)0;
static sw_shape_queue_el_t **global_shape_queue;
static int global_queue_length = 0;
static int global_stats_shape[6];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned long __hash_cb(sw_shape_context_t *shc) {
  return shc->prv_ip;
}

static int __cmp_cb(sw_shape_context_t *shc1, sw_shape_context_t *shc2) {
  return shc1->prv_ip != shc2->prv_ip;
}

int sw_shape_create() {
  global_shape_hash = lh_new(__hash_cb, __cmp_cb,0);
  if (!global_shape_hash) {
    sw_log("%s:%d lh_new() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  sw_shape_stats_null();
  return 0;
}

int sw_shape_destroy() {
  if (sw_debug_flags & SW_DEBUG_SHAPE)
    sw_log("%s:%d destroying shape hash", __FILE__, __LINE__);
  lh_free(global_shape_hash);
  sw_shape_stats_print();
  return 0;
}

int sw_shape_new(uint32_t ip,
                 uint64_t max_bps_in,
                 uint64_t max_bps_out) {
  sw_shape_context_t qshc, *shc;
  spinlock *sp = NULL;
  qshc.prv_ip = ip;
  shc = (sw_shape_context_t *)lh_retrieve(global_shape_hash, (void *)&qshc, &sp);
  if (!shc) {
    spinlock_unlock(sp);
    shc = malloc(sizeof(sw_shape_context_t));
    if (shc == NULL) {
      sw_log("%s:%d malloc() failed for %s: %s", __FILE__, __LINE__,
             sw_pretty_ip(ip), strerror(errno));
      return -1;
    }
    memset(shc, 0, sizeof(sw_shape_context_t));
    shc->prv_ip = ip;
    lh_insert(global_shape_hash, (void *)shc, &sp);

    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d shape context for %s created",
              __FILE__, __LINE__, sw_pretty_ip(ip));
  }
  shc->inward_params.max_bps = max_bps_in;
  shc->outward_params.max_bps = max_bps_out;

  if (sw_debug_flags & SW_DEBUG_SHAPE)
    sw_log("%s:%d shape context for %s updated",
            __FILE__, __LINE__, sw_pretty_ip(ip));
  spinlock_unlock(sp);
  return 0;
}

int sw_shape_del(uint32_t ip) {
  sw_shape_context_t qshc, *shc;
  spinlock *sp = NULL;
  qshc.prv_ip = ip;
  shc = (sw_shape_context_t *)lh_delete(global_shape_hash, (void *)&qshc, &sp);
  spinlock_unlock(sp);
  if (shc) {
    free(shc);
  }
  return 0;
}

int sw_shape_queue_create(int queue_id) {
 // int idx;
  sw_shape_queue_el_t **new_area_ptr; 
  if (!global_queue_length) {
    global_shape_queue = malloc(sizeof(sw_shape_queue_el_t*)*(queue_id + 1));
    if (global_shape_queue == NULL) {
      sw_log("%s:%d malloc() failed for queue %d: %s",
              __FILE__, __LINE__, queue_id, strerror(errno));
      return -1;
    }
    global_queue_length = queue_id + 1;
    memset(global_shape_queue, 0, sizeof(sw_shape_queue_el_t*)*(queue_id + 1));
  }
  else if (global_queue_length < queue_id + 1) {
    new_area_ptr = (sw_shape_queue_el_t **)realloc(global_shape_queue,
                           sizeof(sw_shape_queue_el_t*)*(queue_id + 1));
    if (new_area_ptr == NULL) {
      sw_log("%s:%d realloc() failed for queue %d",
              __FILE__, __LINE__, queue_id);
      return -1;
    }
    memset((void*)new_area_ptr + sizeof(sw_shape_queue_el_t*)*(global_queue_length), 0,
           sizeof(sw_shape_queue_el_t*)*(queue_id + 1 - global_queue_length));
    global_queue_length = queue_id + 1;
    global_shape_queue = new_area_ptr;
  }
  global_shape_queue[queue_id] = malloc(sizeof(sw_shape_queue_el_t));
  if (global_shape_queue[queue_id] == NULL) {
    sw_log("%s:%d malloc() failed entry of queue %d: %s",
            __FILE__, __LINE__, queue_id, strerror(errno));
    return -1;
  }
  memset(global_shape_queue[queue_id], 0, sizeof(sw_shape_queue_el_t*));
  global_shape_queue[queue_id]->curr_time_ptr = 0;
  global_shape_queue[queue_id]->timeslots = malloc(sizeof(sw_shape_timequeue_el_t) * SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH);
  if (global_shape_queue[queue_id]->timeslots == NULL) {
    sw_log("%s:%d malloc() failed for time subqueue of queue %d: %s",
          __FILE__, __LINE__, queue_id, strerror(errno));
    return -1;
  }
  memset(global_shape_queue[queue_id]->timeslots, 0,
         sizeof(sw_shape_timequeue_el_t) * SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH);
  return 0;
}

int sw_shape_queue_destroy() {
  int idx, time_idx, p_idx;
  for (idx = 0; idx < global_queue_length; idx++) {
    if (global_shape_queue[idx]) {
      for(time_idx = 0;
          time_idx < SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH;
          time_idx++){
        for(p_idx = 0; 
            p_idx < global_shape_queue[idx]->timeslots[time_idx].next_free_slot;
            p_idx++) {
          free(global_shape_queue[idx]->timeslots[time_idx].packets[p_idx].packet);
        }
      }
      free(global_shape_queue[idx]->timeslots);
      free(global_shape_queue[idx]);
    }
  }
  free(global_shape_queue);  
  return 0;
}

int sw_shape_schedule(sw_tuple_t *tuple_chain, int length, int rate,
                      char direction, char priority) {
  uint32_t prv_ip;
  sw_shape_context_t qshc, *shc;
  struct timeval now, res, delaytime;
  spinlock *sp = NULL;

  global_stats_shape[SHAPE_SCHEDULE]++;

  if (tuple_chain->protocol != IPPROTO_IP) {
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d unexpected protocol: %s, IP needed", __FILE__, __LINE__,
              sw_pretty_proto(tuple_chain->protocol));
    return -1;
  }
  prv_ip = ntohl(((sw_tuple_ip_t*)(tuple_chain->prv))->ip);
  qshc.prv_ip = prv_ip;
  shc = (sw_shape_context_t *)lh_retrieve(global_shape_hash, (void *)&qshc, &sp);
  if (!shc) {
    spinlock_unlock(sp);
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d no shape context for %s", __FILE__, __LINE__,
              sw_pretty_ip(prv_ip));
    global_stats_shape[DROP_SHAPE_NO_CONTEXT]++;
    return -1;
  }
  if (gettimeofday(&now, NULL)) {
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d gettimeifday() failed: %s", 
             __FILE__, __LINE__, strerror(errno));
    spinlock_unlock(sp);
    return -1;
  }
  switch(direction) {
    case SW_SHAPE_IN:
      delaytime.tv_usec = length<40 ? 0 : ((length-40) * 8 * 10000)/(rate*10);
/*     sw_log("%s:%d dir %d - last_l %d, l %d, max_rate %llu bps, delay %lu us",
              __FILE__, __LINE__, SW_SHAPE_IN,
              shc->inward_params.last_packet_length, length,
              rate, delaytime.tv_usec); */
      shc->inward_params.last_packet_length = length;
      delaytime.tv_sec = delaytime.tv_usec/1000000;
      delaytime.tv_usec = delaytime.tv_usec%1000000;
      if (!shc->inward_params.last_packet_time.tv_sec) {
        shc->inward_params.last_packet_time.tv_sec = now.tv_sec;
        shc->inward_params.last_packet_time.tv_usec = now.tv_usec;
      } else {
        tv_add(&res, &shc->inward_params.last_packet_time, &delaytime);
        shc->inward_params.last_packet_time.tv_sec = res.tv_sec;
        shc->inward_params.last_packet_time.tv_usec = res.tv_usec;
      }
      if (tv_cmp(&shc->inward_params.last_packet_time, &now) == 1) {
        tv_sub(&res, &shc->inward_params.last_packet_time, &now);
        spinlock_unlock(sp);
        return tv2usec(&res);
      } else {
        shc->inward_params.last_packet_time.tv_sec = now.tv_sec;
        shc->inward_params.last_packet_time.tv_usec = now.tv_usec;
      }
      break;
    case SW_SHAPE_OUT:
      delaytime.tv_usec = length<40 ? 0 : ((length-40) * 8 * 10000)/(rate*10);
/*     sw_log("%s:%d dir %d - last_l %d, l %d, max_rate %llu bps, delay %lu us",
              __FILE__, __LINE__, SW_SHAPE_OUT,
              shc->outward_params.last_packet_length, length, 
              rate, delaytime.tv_usec);   */
      shc->outward_params.last_packet_length = length;
      delaytime.tv_sec = delaytime.tv_usec/1000000;
      delaytime.tv_usec = delaytime.tv_usec%1000000;
      if (!shc->outward_params.last_packet_time.tv_sec) {
        shc->outward_params.last_packet_time.tv_sec = now.tv_sec;
        shc->outward_params.last_packet_time.tv_usec = now.tv_usec;
      } else {
        tv_add(&res, &shc->outward_params.last_packet_time, &delaytime);
        shc->outward_params.last_packet_time.tv_sec = res.tv_sec;
        shc->outward_params.last_packet_time.tv_usec = res.tv_usec;
      }
      if (tv_cmp(&shc->outward_params.last_packet_time, &now) == 1) {
        tv_sub(&res, &shc->outward_params.last_packet_time, &now);
        spinlock_unlock(sp);
        return tv2usec(&res);
      } else {
        shc->outward_params.last_packet_time.tv_sec = now.tv_sec;
        shc->outward_params.last_packet_time.tv_usec = now.tv_usec;
      }
      break;
    default:
      if (sw_debug_flags & SW_DEBUG_SHAPE)
        sw_log("%s:%d unknown shape direction", __FILE__, __LINE__);
      spinlock_unlock(sp);
      return -1;
  }
  spinlock_unlock(sp);
  return 0;
}

int sw_shape_delay(int queue_id, int delaytime_usec, 
                   sw_memgmt_buf_t *packet, int length) {
  int next_free_slot, idx, time_curr, time_off;
  sw_shape_timequeue_el_t *q4delay;

  if (delaytime_usec >= SW_SHAPE_PACKET_TIME_STORE_MAX) {
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d delaytime is too much: %d usec", 
              __FILE__, __LINE__, delaytime_usec);
    return -1;
  }
  pthread_mutex_lock(&mutex);
  time_curr = global_shape_queue[queue_id]->curr_time_ptr;
  time_off = delaytime_usec / SW_SHAPE_QUEUE_PROCESS_FRQ;
  idx = time_curr + time_off;
  if (idx >= SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH)
    idx -= SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH;
/*  sw_log("%s:%d queue %d, delay %d us, subqueue time[%d]",
          __FILE__, __LINE__, queue_id, delaytime_usec, time_off); */
  if (!(global_shape_queue[queue_id] && 
        global_shape_queue[queue_id]->timeslots)) {
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d no appropriate queue found, smth bad happens",
              __FILE__, __LINE__, queue_id);
    pthread_mutex_unlock(&mutex);
    return -1;
  }
  q4delay = &global_shape_queue[queue_id]->timeslots[idx];
  if (q4delay->next_free_slot >= SW_SHAPE_PACKET_PROCESS_MAX) {
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d current timeslot(%d) is full", __FILE__, __LINE__, idx);
    global_stats_shape[DROP_SHAPE_TIMESLOT_FULL]++;
    pthread_mutex_unlock(&mutex);
    return -1;
  }
  next_free_slot = q4delay->next_free_slot;
/*  sw_log("%s:%d queue_id %d, idx %d, next_free_slot %d", __FILE__, __LINE__,
          queue_id, idx, next_free_slot); */
  q4delay->packets[next_free_slot].length = length;
  q4delay->packets[next_free_slot].packet = packet;
  q4delay->next_free_slot++;
  pthread_mutex_unlock(&mutex);
  global_stats_shape[SHAPE_DELAY]++;
  return 0;
}

int sw_shape_queue_process(sw_socket_handler_t *h, int a_qid) {
  int packet_idx, idx, time_p, qid; 
  sw_shape_timequeue_el_t *q2process;

  qid = a_qid;
  time_p = global_shape_queue[qid]->curr_time_ptr;
  pthread_mutex_lock(&mutex);
  if (time_p == SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH-1)
    global_shape_queue[qid]->curr_time_ptr = 0;
  else
    global_shape_queue[qid]->curr_time_ptr++;
  pthread_mutex_unlock(&mutex);

  if (!(global_shape_queue[qid] && 
        global_shape_queue[qid]->timeslots)) {
    global_stats_shape[DROP_SHAPE_QUEUE]++;
    if (sw_debug_flags & SW_DEBUG_SHAPE)
      sw_log("%s:%d no appropriate queue found, smth bad happens",
              __FILE__, __LINE__);
    return -1;
  }
  q2process = &global_shape_queue[qid]->timeslots[time_p];  
  while (q2process->next_free_slot) {
    packet_idx = q2process->next_free_slot - 1; 
    if (h->send(h, q2process->packets[packet_idx].packet->buf,
                   q2process->packets[packet_idx].length) == -1) {
      if (sw_debug_flags & SW_DEBUG_SHAPE)
        sw_log("%s:%d failed to send packet (from queue %d)",
               __FILE__, __LINE__, qid);
      global_stats_shape[DROP_SHAPE_QUEUE_SEND_FAILED]++;
    }
/*  sw_log("%s:%d cloned packet for sending", __FILE__, __LINE__); */
    q2process->next_free_slot--;
    sw_memgmt_free_buf(q2process->packets[packet_idx].packet->idx);
  }
  return 0;
}

int sw_shape_stats_null() {
  global_stats_shape[DROP_SHAPE_QUEUE] = 0;
  global_stats_shape[DROP_SHAPE_NO_CONTEXT] = 0;
  global_stats_shape[DROP_SHAPE_TIMESLOT_FULL] = 0;
  global_stats_shape[DROP_SHAPE_QUEUE_SEND_FAILED] = 0;
  global_stats_shape[SHAPE_SCHEDULE] = 0;
  global_stats_shape[SHAPE_DELAY] = 0;
  return 0;
}

int sw_shape_stats_print() {
  sw_log("%s:%d SHAPE_drops: queue %d, schedule(context) %d, delay(timeslot full) %d, send %d", __FILE__, __LINE__,
          global_stats_shape[DROP_SHAPE_QUEUE],
          global_stats_shape[DROP_SHAPE_NO_CONTEXT],
          global_stats_shape[DROP_SHAPE_TIMESLOT_FULL],
          global_stats_shape[DROP_SHAPE_QUEUE_SEND_FAILED]);
  sw_log("%s:%d SHAPE_throughput: total %d, delayed %d",  __FILE__, __LINE__,
          global_stats_shape[SHAPE_SCHEDULE],
          global_stats_shape[SHAPE_DELAY]);
  return 0;
}
