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

#ifndef __SW_SHAPE_H__
#define __SW_SHAPE_H__

#include "tuple.h"
#include "sweetspot.h"
#include "memgmt.h"

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <time.h>

#define SW_SHAPE_IN         1
#define SW_SHAPE_OUT        2

#define SW_SHAPE_PACKET_TIME_STORE_MAX 9000000  /* in microseconds */ 
#define SW_SHAPE_QUEUE_PROCESS_FRQ     1000   /* in microseconds */
#define SW_SHAPE_GLOBAL_TIME_QUEUE_LENGTH 9000
#define SW_SHAPE_PACKET_PROCESS_MAX 2048 

typedef struct __sw_shape_params_t {
  uint64_t max_bps;
  int last_packet_length;
  struct timeval last_packet_time;
} sw_shape_params_t;

typedef struct __sw_shape_context_t {
  uint32_t prv_ip;
  sw_shape_params_t inward_params;
  sw_shape_params_t outward_params; 
} sw_shape_context_t;

typedef struct __sw_shape_packet_t {
  sw_memgmt_buf_t *packet;
  int length;
} sw_shape_packet_t;

typedef struct __sw_shape_timequeue_el_t {
  int next_free_slot;
  sw_shape_packet_t packets[SW_SHAPE_PACKET_PROCESS_MAX];
} sw_shape_timequeue_el_t;

typedef struct __sw_shape_queue_el_t {
  int curr_time_ptr;
  sw_shape_timequeue_el_t *timeslots;
} sw_shape_queue_el_t;


#ifdef __cplusplus
extern "C" {
#endif

extern int sw_shape_create PROTO(());
extern int sw_shape_destroy PROTO(());
extern int sw_shape_new PROTO((uint32_t ip,
                               uint64_t max_bps_in,
                               uint64_t max_bps_out));
extern int sw_shape_del PROTO((uint32_t ip));
extern int sw_shape_queue_create PROTO((int queue_id));
extern int sw_shape_queue_destroy PROTO(());
extern int sw_shape_schedule PROTO((sw_tuple_t *tuple_chain,
                                    int length,
                                    int rate,
                                    char direction,
                                    char priority));
extern int sw_shape_delay PROTO((int queue_id,
                                 int delaytime_usec,
                                 sw_memgmt_buf_t *packet, 
                                 int length));
extern int sw_shape_queue_process PROTO((sw_socket_handler_t *h, int a_qid) );
extern int sw_shape_stats_null PROTO(());
extern int sw_shape_stats_print PROTO(());


#ifdef __cplusplus
}
#endif

#endif
