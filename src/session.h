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
#ifndef __SW_SESSION_H__
#define __SW_SESSION_H__

#include "portab.h"

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <sys/time.h>
#include "netset.h"
#include "spinlock.h"

#define SW_SESSION_ST_CAPTURED  1
#define SW_SESSION_ST_RELEASED  2

#define SW_SESSION_TERM_TIME    1
#define SW_SESSION_TERM_IDLE    2
#define SW_SESSION_TERM_VOLUME  3
#define SW_SESSION_TERM_ADMIN   4
#define SW_SESSION_TERM_USER    5

#define SW_SESSION_ACCT_INTERIM_INTERVAL  "300"

#define SW_SESSION_DEFAULT_RETENTION  3600

#define SW_SESSION_DOWNSTREAM_RC  "/usr/local/etc/sweetspot/rc/downstream.sh"
#define SW_SESSION_UPSTREAM_RC    "/usr/local/etc/sweetspot/rc/upstream.sh"

typedef struct __sw_session_entry_t {
  uint32_t ip;
  /* session attributes */
  int state;
  uint32_t session_id;
  char *session_context;
  char *dn_filter_name;
  int filter_id;
  int termination_cause;
  time_t interim_interval;
  /* timings */
  time_t deathday;
  time_t interim; /* Next interim accting time */
  time_t retention; /* For how long this session will persist when DN */
  /* modification synchronization */
  int stateid;
} sw_session_entry_t;

extern int global_debug_flag;

extern sw_netset_t *global_ns;
extern sw_netset_t * local_ip_ns;
extern sw_netset_t * snat_ip_ns;

#ifdef __cplusplus
extern "C" {
#endif
  extern int sw_session_create PROTO(());
  extern int sw_session_destroy();
  extern int sw_session_new PROTO((sw_session_entry_t **n, uint32_t ip, spinlock ** sp));
  extern int sw_session_del PROTO((uint32_t ip));
  extern int sw_session_stop PROTO((uint32_t ip,
                                    char *filter_name,
                                    int termination_cause,
                                    char *session_context,
                                    char *event_context));
  extern int sw_session_start PROTO((uint32_t ip,
                                     char *filter_name,
				     int interim_interval,
                                     char *session_context,
                                     char *event_context));
  extern int sw_session_status PROTO((uint32_t ip,
                                      int *state,
                                      uint32_t *session_id,
                                      char **filter_name,
                                      time_t *interim_interval,
                                      char **session_context,
                                      int *termination_cause));

  extern int sw_session_status_nonblock PROTO((uint32_t ip,
                                      int *state,
                                      uint32_t *session_id,
                                      char **filter_name,
                                      time_t *interim_interval,
                                      char **session_context,
                                      int *termination_cause));
  extern int sw_session_acquire_stateid PROTO((uint32_t ip,
                                                int *stateid,
                                                int modified));
  extern int sw_session_verify_stateid PROTO((uint32_t ip,
                                               int *stateid));
  extern int sw_session_get_filter_id PROTO((uint32_t ip));
  extern int sw_session_retention PROTO((uint32_t ip,
                                             time_t *retention));
  extern int sw_session_expire PROTO(());

#ifdef __cplusplus
}
#endif

#endif
