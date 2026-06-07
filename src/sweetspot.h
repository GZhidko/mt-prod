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
#ifndef __SWEETSPOT_H__
#define __SWEETSPOT_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "portab.h"
#include <pthread.h>
#include <netinet/in.h>
#include "cache.h"
#include "mutexCirularBuffer.h"



extern int global_debug_flag;
#define MAX_PROC_RCV_CNT "4"
#define MAX_MUTEX_BY_PEER 1000
//#define CHECHSUMM_FULL_CALC

extern int global_alive_flag;

extern int process_qnty;


#define SW_SWEETSPOT_CONFIG_FILE SW_DEFAULT_CONFIG_DIR"/sweetspot.conf"
#define SW_SWEETSPOT_LOG_FILE "/var/log/sweetspot.log"
#define SW_SWEETSPOT_STATS_INT "600"
//#define AF_PACK_SEND_IMP
//#define USE_SELCT_FOR_UAM

extern char inner_gw_mac[8];
extern char outer_gw_mac[8];


//extern pthread_mutex_t sh_lock;


typedef struct __sw_socket_handler_t {
  CONST char *name;
  int socket;
  int ifindex;
  StaticCache * cache;
  CircularBufferGauge * cBuf;
  spinlock * sp;
  int (*create)(struct __sw_socket_handler_t *self);
  int (*cconf_out)(struct __sw_socket_handler_t *self, int interface, int if_idx);
  int (*destroy)(struct __sw_socket_handler_t *self);
  int (*recv)(struct __sw_socket_handler_t *self, char * buf,struct sockaddr_in * addr, char * cyphertext );
  int (*proc)(struct __sw_socket_handler_t *self, char * buf, int length,struct sockaddr_in * addr, char * cyphertext );
  int (*send)(struct __sw_socket_handler_t *self, char *packet, int length);
  void *priv;
} sw_socket_handler_t;


extern sw_socket_handler_t ** global_socket_handlers;








#endif
