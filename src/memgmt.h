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

#ifndef __SW_MEMGMT_H__
#define __SW_MEMGMT_H__

#define SW_MEMGMT_BUF_NUM   10000
#define SW_MEMGMT_BUF_SIZE  1500

typedef struct __sw_memgmt_buf_t {
  int idx;
  char buf[SW_MEMGMT_BUF_SIZE];
} sw_memgmt_buf_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int sw_memgmt_create PROTO(());
extern int sw_memgmt_get_buf PROTO((sw_memgmt_buf_t **buf, int bufsize));
extern int sw_memgmt_free_buf PROTO((int idx));
extern int sw_memgmt_stats_print PROTO(());

#ifdef __cplusplus
}
#endif

#endif
