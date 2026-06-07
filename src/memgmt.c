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
#include <pthread.h>
#include "debug.h"
#include "memgmt.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/* XXX static int buf_num = atoi(sw_config_get("packet-buf-num", SW_MEMGMT_BUF_NUM)); */
static int free_buf_idx[SW_MEMGMT_BUF_NUM];
static sw_memgmt_buf_t bufs[SW_MEMGMT_BUF_NUM];
static int last_free_buf = SW_MEMGMT_BUF_NUM - 1;
pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

int sw_memgmt_create() {
  int idx;
  for (idx = 0; idx < SW_MEMGMT_BUF_NUM; idx++) {
    bufs[idx].idx = idx;
    free_buf_idx[idx] = idx;
  }
  return 0;
}

int sw_memgmt_get_buf(sw_memgmt_buf_t **buf, int bufsize) {
  int idx;
  if (bufsize > 1500) {
    return -1;
  }
  pthread_mutex_lock(&buf_mutex);
  if (last_free_buf == -1) {
    sw_log("%s:%d no buf available", __FILE__, __LINE__);
    pthread_mutex_unlock(&buf_mutex);
    return -1;
  }
  idx = free_buf_idx[last_free_buf--];
  *buf = &bufs[idx];
  pthread_mutex_unlock(&buf_mutex);
  return 0;
}

int sw_memgmt_free_buf(int idx) {
  pthread_mutex_lock(&buf_mutex);
  if (idx >= SW_MEMGMT_BUF_NUM || last_free_buf == SW_MEMGMT_BUF_NUM - 1) {
    pthread_mutex_unlock(&buf_mutex);
    return -1;
  }
  free_buf_idx[++last_free_buf] = idx; 
  pthread_mutex_unlock(&buf_mutex);
  return 0;
}

int sw_memgmt_stats_print() {
  sw_log("%s:%d %d free bufs", __FILE__, __LINE__, last_free_buf);
  return 0;
}
