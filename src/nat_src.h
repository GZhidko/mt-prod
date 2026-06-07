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
#ifndef __SW_NAT_SRC_H__
#define __SW_NAT_SRC_H__

#include <stdint.h>
#include "tuple.h"
#include "portab.h"
#include "spinlock.h"

#define SW_NAT_SRC_RELATIONS_MAX       11048575
#define RELATION_EXPIRE_INTERVAL 1800
#define NUM_GARBAGE_THREAD 1
/*
** Refresh active relation when it gets this far to expiration point.
** Must be non-zero positive.
*/
#define SW_NAT_SRC_RELATIONS_AGE_MIN   SW_NAT_SRC_RELATIONS_MAX/2
#define ACTIVE_REL_PROTECT_SEC 10
#define ASSIGN_RETRY_MAX 128



typedef struct __sw_nat_src_relation_t {
  sw_tuple_t *prv;
  sw_tuple_t *pub;
  uint32_t seq;
  time_t last_dl_time;
} sw_nat_src_relation_t;

extern int sw_nat_src_enabled_flag;
extern int global_debug_flag;
extern spinlock sp_gst;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_nat_src_create PROTO(());
  extern int sw_nat_src_do PROTO((sw_tuple_t **pub, sw_tuple_t *prv));
  extern int sw_nat_src_undo PROTO((sw_tuple_t **prv, sw_tuple_t *pub));
  extern void PROTO(sw_nat_src_stop_garbage());

#ifdef __cplusplus
}
#endif

#endif
