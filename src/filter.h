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
#ifndef __SW_FILTER_H__
#define __SW_FILTER_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "portab.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

//#include "sweetspot.h"

extern int global_debug_flag;
extern pthread_rwlock_t global_filter_lock;

#define SW_FILTER_DIR          "/usr/local/etc/sweetspot/filters"

#define SW_FILTER_NAME_ANON    "anonymous"

#define SW_FILTER_SLOTS_MAX   512
#define SW_FILTER_PORTS_MAX   8

#define SW_FILTER_RELOAD_INTERVAL   "3600"

#define SW_FILTER_ID_NONE      0x7fffffff

#define SW_FILTER_DIR_IN       0x01
#define SW_FILTER_DIR_OUT      0x02

#define SW_FILTER_ACTION_PASS  0x01
#define SW_FILTER_ACTION_BLOCK 0x02
#define SW_FILTER_ACTION_DNAT  0x04
#define SW_FILTER_ACTION_SHAPE 0x08

#define SW_FILTER_TARGET_EQ    0x01
#define SW_FILTER_TARGET_NE    0x02
#define SW_FILTER_TARGET_LT    0x04
#define SW_FILTER_TARGET_LE    0x08
#define SW_FILTER_TARGET_GT    0x10
#define SW_FILTER_TARGET_GE    0x20
#define SW_FILTER_TARGET_RG    0x40

#define SW_FILTER_FLAGS_FIN        0x01
#define SW_FILTER_FLAGS_SYN        0x02
#define SW_FILTER_FLAGS_RST        0x04
#define SW_FILTER_FLAGS_PUSH       0x08
#define SW_FILTER_FLAGS_ACK        0x10
#define SW_FILTER_FLAGS_URG        0x20

typedef struct __sw_filter_target_t {
  uint32_t ip;
  uint32_t mask;
  uint16_t ports[SW_FILTER_PORTS_MAX];
  uint16_t port_lo;
  uint16_t port_hi;
  int op;
  char port_cnt;
} sw_filter_target_t;

typedef struct __sw_filter_flags_t {
  char set;
  char mask;
} sw_filter_flags_t;

typedef struct __sw_filter_dnat_t {
  uint32_t ip;
  uint16_t port;
} sw_filter_dnat_t;

typedef struct __sw_filter_shape_t {
  int rate;
} sw_filter_shape_t;

typedef struct __sw_filter_rule_t {
  sw_filter_target_t src;
  sw_filter_target_t dst;
  sw_filter_flags_t flags;
  sw_filter_dnat_t dnat;
  sw_filter_shape_t shape;
  uint8_t proto;
  int dir;
  int action;
  int num;
  struct __sw_filter_rule_t *next;
} sw_filter_rule_t;

typedef struct __sw_filter_set_t {
  char *name;
  time_t mtime;
  sw_filter_rule_t *ruleset_in;
  sw_filter_rule_t *ruleset_out;
} sw_filter_set_t;

typedef struct __sw_filter_decision_t {
  int id;
  sw_filter_dnat_t dnat;
} sw_filter_decision_t;

typedef struct __sw_filter_fileinfo_t {
  char *fullpath;
  char *filename;
  time_t mtime;
} sw_filter_fileinfo_t;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_filter_create PROTO((CONST char *dir_name));
  extern int sw_filter_reload PROTO((CONST char *dir_name, time_t now));
  extern int sw_filter_destroy PROTO((sw_filter_set_t *filter));
  extern int sw_filter_get PROTO((sw_filter_rule_t **ruleset_in, 
                                  sw_filter_rule_t **ruleset_out,
                                  int filter_id));
  extern int sw_filter_get_id PROTO((CONST char *name));
  extern int sw_filter_get_name(CONST char **name, int idx);
  extern int sw_filter_debug PROTO((char *name));
  extern int sw_filter_parse_ruleset PROTO ((sw_filter_rule_t **rs_in, 
                                             sw_filter_rule_t **rs_out,
                                             char *text));
#ifdef __cplusplus
}
#endif

#endif
