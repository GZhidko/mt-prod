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
#ifndef __SW_UAMCLNT_H__
#define __SW_UAMCLNT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "portab.h"

#define SW_UAM_CLIENT_RECV_TIMEOUT "1"

#define SW_SWEETUAM_CONFIG_FILE SW_DEFAULT_CONFIG_DIR"/sweetuam.conf"

extern CONST char *sw_uamclt_default_config_file;

typedef struct __sw_uamclt_member_t {
  struct sockaddr_in endpoint;
  struct __sw_uamclt_member_t *next;
} sw_uamclt_member_t;

typedef struct __sw_uamclt_group_t {
  int sock;
  struct sockaddr_in endpoint;
  sw_uamclt_member_t *members;
} sw_uamclt_group_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif
  extern int sw_uam_create_client PROTO((sw_uamclt_group_t **group));
  extern int sw_uam_destroy_client PROTO((sw_uamclt_group_t *group));
  extern int sw_uam_send_msg PROTO((sw_uamclt_group_t *group, char **argv));
  extern int sw_uam_recv_msg PROTO((sw_uamclt_group_t *group, char **argv));
#ifdef __cplusplus
}
#endif

#endif
