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
#ifndef __SW_IF_H__

#include "portab.h"
#include "lhash.h"
#include "session.h"
extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_if_open_for_input PROTO((CONST char *ifname, int fanout_group_id, int * if_idx));
  extern int sw_if_open_for_output PROTO((CONST char *ifname));
  extern int sw_if_close PROTO((int sock));
  extern int sw_if_send PROTO((int fd, char *packet, int length, char isinner, int ifindex));
  extern int sw_if_recv PROTO((int fd, char *packet, int length));
  extern int getMACFromIP PROTO((const char* ip_address,const char *  ifname, char* mac_address));
#endif
