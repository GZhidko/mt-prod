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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "uammsg.h"
#include "pretty.h"
#include "cfg.h"
#include "uamclt.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static time_t global_recv_timeout;

CONST char *sw_uamclt_default_config_file = SW_SWEETUAM_CONFIG_FILE;

int sw_uam_create_client(sw_uamclt_group_t **__group) {
  sw_uamclt_group_t *group;
  sw_uamclt_member_t *member;
  char *uam_server_group;
  char *server, *port;

  group = (sw_uamclt_group_t *)malloc(sizeof(sw_uamclt_group_t));
  if (!group) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  memset(group, 0, sizeof(sw_uamclt_group_t));
  group->endpoint.sin_family = AF_INET;
  group->endpoint.sin_addr.s_addr = inet_addr(sw_config_get("uam-client-address", "0.0.0.0"));

  group->sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (group->sock == -1) {
    sw_log("%s:%d socket() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d client socket created %d",
           __FILE__, __LINE__, group->sock);

  if (bind(group->sock, (struct sockaddr *)&(group->endpoint), 
           sizeof(group->endpoint)) == -1) {
    sw_log("%s:%d bind() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d sock %d bound to %s:%s",
           __FILE__, __LINE__, group->sock,
           sw_pretty_ip(ntohl(group->endpoint.sin_addr.s_addr)), 
           sw_pretty_port(ntohs(group->endpoint.sin_port)));

  uam_server_group = strdup(sw_config_get("uam-server-group", ""));
  if (!uam_server_group) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  server = strtok(uam_server_group, ",");
  while(server) {
    member = (sw_uamclt_member_t *)malloc(sizeof(sw_uamclt_member_t));
    if (!member) {
      sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
      free(uam_server_group);
      return -1;
    }
    memset(member, 0, sizeof(sw_uamclt_member_t));

    port = server;
    while(*port && *port != ':') port++;
    if (*port == ':') {
      *port = '\0';
      member->endpoint.sin_port = htons(atoi(++port));
    } else {
      member->endpoint.sin_port = htons(3993);
    }
    member->endpoint.sin_addr.s_addr = inet_addr(server);  
    member->endpoint.sin_family = AF_INET;

    if (group->members) {
      member->next = group->members;
    }
    group->members = member;

    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d added server group member %s:%s",  __FILE__, __LINE__, 
             sw_pretty_ip(ntohl(member->endpoint.sin_addr.s_addr)), 
             sw_pretty_port(ntohs(member->endpoint.sin_port)));

    server = strtok(NULL, ",");
  }

  free(uam_server_group);

  if (group->members == NULL) {
    sw_log("%s:%d not uam-server-group configured", __FILE__, __LINE__);
    sw_uam_destroy_client(group);
    return -1;
  }

  global_recv_timeout = atoi(sw_config_get("uam-client-timeout",
                                           SW_UAM_CLIENT_RECV_TIMEOUT));

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d UAM client recv timeout %d%s", 
           __FILE__, __LINE__, global_recv_timeout,
           global_recv_timeout < 100 ? "sec" : "usec");

  *__group = group;

  return 0;
}

int sw_uam_destroy_client(sw_uamclt_group_t *group) {
  sw_uamclt_member_t *member;

  close(group->sock);

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d client socket destroyed %d",
           __FILE__, __LINE__, group->sock);

  member = group->members;
  while(member) {
    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d group member %s:%s freed",  __FILE__, __LINE__, 
             sw_pretty_ip(ntohl(member->endpoint.sin_addr.s_addr)), 
             sw_pretty_port(ntohs(member->endpoint.sin_port)));
    group->members = member;
    member = member->next;
    free(group->members);
  }
  free(group); 

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d server group freed", __FILE__, __LINE__);

  return 0;
}

int sw_uam_send_msg(sw_uamclt_group_t *group, char **argv) {
  sw_uamclt_member_t *member;
  char *cyphertext;
  int rc;

  rc = sw_uam_build_msg(argv, &cyphertext);
  if (rc == -1)
    return -1;

  for(member=group->members; member; member=member->next) {
    rc = sendto(group->sock, cyphertext, rc, 0,
                (struct sockaddr *)&member->endpoint,
                sizeof(member->endpoint));
    if (rc == -1) {
      sw_log("%s:%d send() failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
    }

    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d sent UAM msg: %d octets to %s:%s",
             __FILE__, __LINE__, rc,
             sw_pretty_ip(ntohl(member->endpoint.sin_addr.s_addr)),
             sw_pretty_port(ntohs(member->endpoint.sin_port)));
  }

  return 0;
}

int sw_uam_recv_msg(sw_uamclt_group_t *group, char **argv) {
  char cyphertext[SW_UAM_MSG_SIZE];
  fd_set fds;
  struct timeval tout;
  int rc;

  tout.tv_sec = global_recv_timeout < 100 ? global_recv_timeout : 0;
  tout.tv_usec = global_recv_timeout < 100 ? 0 : global_recv_timeout;

  FD_ZERO(&fds);
  FD_SET(group->sock, &fds);

  switch(select(group->sock+1, &fds, NULL, NULL, &tout)) {
  case -1:
    sw_log("%s:%d select() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  case 0:
    sw_log("%s:%d select() timeout", __FILE__, __LINE__);
    return -1;
  default:
    break;
  }

  rc = recv(group->sock, &cyphertext, sizeof(cyphertext), 0);
  if (rc == -1) {
    sw_log("%s:%d recv() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d recved UAM msg: %d octets", __FILE__, __LINE__, rc);

  rc = sw_uam_parse_msg(argv, cyphertext, rc);
  if (rc == -1)
    return -1;

  return 0;
}
