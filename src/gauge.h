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
#ifndef __SW_GAUGE_H__
#define __SW_GAUGE_H__

#include "portab.h"
#include "tuple.h"

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <sys/time.h>
#include "mutexCirularBuffer.h"

#define SW_GAUGE_MAX_OCTETS_IN  0xfffffffffff70000ll /* unlimited */
#define SW_GAUGE_MAX_OCTETS_OUT 0xfffffffffff70000ll
#define SW_GAUGE_MAX_BPS_IN     0xfffffffffff70000ll /* unlimited */
#define SW_GAUGE_MAX_BPS_OUT    0xfffffffffff70000ll
#define SW_GAUGE_MAX_DURATION   3600
#define SW_GAUGE_MAX_IDLE       600

#define SW_GAUGE_RATE_INTERVAL  "5"

#define SW_GAUGE_DIR_IN         1
#define SW_GAUGE_DIR_OUT        2

#define NUM_GAUGE_THREAD 0

typedef struct __sw_gauge_entry_t {
  uint32_t ip;
  /* limits */
  uint64_t max_octets_in;
  uint64_t max_octets_out;
  uint64_t max_bps_in;
  uint64_t max_bps_out;
  time_t max_duration;
  time_t max_idle;
  /* currents */
  uint64_t octets_in;
  uint64_t octets_out;
  uint64_t bps_in;
  uint64_t bps_out;
  uint64_t packets_in;
  uint64_t packets_out;
  /* peaks */
  uint64_t peak_bps_in;
  uint64_t peak_bps_out;
  /* timings */
  time_t deathday;
  time_t idle;
  /* throughput measurement */
  uint64_t bps_probe_in;
  uint64_t bps_probe_out;
  time_t bps_probe_time;
} sw_gauge_entry_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif
  extern int sw_gauge_create PROTO(());
  extern int sw_gauge_destroy PROTO(());
  extern int sw_gauge_new PROTO((uint32_t ip,
                                 uint64_t max_octets_in,
                                 uint64_t max_octets_out,
                                 uint64_t max_bps_in,
                                 uint64_t max_bps_out,
                                 time_t max_duration,
                                 time_t max_idle));
  extern int sw_gauge_del PROTO((uint32_t ip, uint8_t blocking));
  extern int sw_gauge_update PROTO((sw_tuple_t *tuple_chain,
                                    uint64_t octets, int direction));
  extern int sw_gauge_limits PROTO((uint32_t ip, 
                                    uint64_t *max_octets_in, 
                                    uint64_t *max_octets_out,
                                    uint64_t *max_bps_in, 
                                    uint64_t *max_bps_out,
                                    time_t *max_duration, 
                                    time_t *max_idle,int block));
  extern int sw_gauge_current PROTO((uint32_t ip, 
                                     uint64_t *octets_in, 
                                     uint64_t *octets_out,
                                     uint64_t *packets_in, 
                                     uint64_t *packets_out,
                                     uint64_t *bps_in, 
                                     uint64_t *bps_out,
                                     time_t *duration, 
                                     time_t *idle,int block));
  extern int sw_gauge_peak PROTO((uint32_t ip, 
                                  uint64_t *bps_in, 
                                  uint64_t *bps_out,int block));
  extern int sw_gauge_expire PROTO(());

  extern int sw_gauge_drop_stat PROTO((CircularBufferGauge * cb, sw_tuple_t *tuple_chain, uint64_t octets, int direction));
  extern void gauge_buffer_register PROTO((CircularBufferGauge * buf));

#ifdef __cplusplus
}
#endif

#endif
