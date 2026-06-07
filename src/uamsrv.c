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
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "sweetspot.h"
#include "nat_src.h"
#include "nat_src_endpoint_ip.h"
#include "nat_src_endpoint_udp.h"
#include "uammsg.h"
#include "session.h"
#include "gauge.h"
#include "pretty.h"
#include "cfg.h"
#include "uamsrv.h"
#include "log.h"
#include <fcntl.h>
#include "shape.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int global_report_error_flag = 0;

static int __create(sw_socket_handler_t *self) {
  struct sockaddr_in endpoint;

  self->socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (self->socket == -1) {
    sw_log("%s:%d socket() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d server socket created, fd #%d", __FILE__, __LINE__, 
           self->socket);

  memset(&endpoint, 0, sizeof(endpoint));
  endpoint.sin_family = AF_INET;
  endpoint.sin_addr.s_addr = inet_addr(sw_config_get("uam-server-address", 
                                                     "0.0.0.0"));
  endpoint.sin_port = ntohs(atoi(sw_config_get("uam-server-port", "3993")));

  int flags = fcntl(self->socket, F_GETFL, 0);
  if (flags == -1) {
      sw_log("%s:%d fcntl(F_GETFL) failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
  }
#ifndef USE_SELCT_FOR_UAM
  if (fcntl(self->socket, F_SETFL, flags | O_NONBLOCK) == -1) {
      sw_log("%s:%d NONBLOCK() failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
  }
#else
  if (fcntl(self->socket, F_SETFL, flags) == -1) {
      sw_log("%s:%d NONBLOCK() failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
  }
#endif
  if (bind(self->socket, (struct sockaddr *)&endpoint, 
           sizeof(endpoint)) == -1) {
    sw_log("%s:%d bind() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d sock %d bound to %s:%d", __FILE__, __LINE__, 
           self->socket, sw_pretty_ip(ntohl(endpoint.sin_addr.s_addr)), 
           ntohs(endpoint.sin_port));

  global_report_error_flag = atoi(sw_config_get("uam-report-errors", "0"));

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d will %s errors", 
           __FILE__, __LINE__, global_report_error_flag ? "report" : "mute");

  return 0;
}

static int __destroy(sw_socket_handler_t *self) {
  close(self->socket);
  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d server socket fd #%d destroyed", __FILE__, __LINE__, 
           self->socket);
  return 0;
}

static int __recv(sw_socket_handler_t *self, char * buf, struct sockaddr_in * addr, char * cyphertext ) {
    struct sockaddr_in addr_;
    socklen_t addr_size = sizeof(addr_);
    return recvfrom(self->socket, cyphertext,
                  SW_UAM_MSG_SIZE-1, 0,
                  addr, &addr_size);

}

static int __proc(sw_socket_handler_t *self, char * buf, int length, struct sockaddr_in * addr, char * cyphertext) {


  struct sockaddr_in addr_ = *addr;
  //char cyphertext[SW_UAM_MSG_SIZE];
  char *argv[SW_UAM_ARG_MAX], *p;
  char nbufs[SW_UAM_ARG_MAX][SW_UAM_ARG_SIZE];
  char serial[SW_UAM_ARG_SIZE];
  int argc, state, idx, rc;
  socklen_t addr_size = sizeof(addr_);
  char *context, *filter_name;
  uint64_t octets_in, octets_out, bps_in, bps_out;
  uint32_t ip, session_id;
  int stateid = 0;
  time_t duration, idle, interim_interval;
  
  rc = length;
  if (rc == 0) return 0;
  if (rc == -1) {
    sw_log("%s:%d recvfrom() failed: %s", __FILE__, __LINE__,
           strerror(errno));
    return -1;
  }
  
  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d read cypher UAM of %d octets", __FILE__, __LINE__, rc);

  if (rc == SW_UAM_MSG_SIZE-1) {
    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d UAM msg too large (%d octets)", __FILE__, __LINE__, rc);
    return 0; /* Drop */
  }

  rc = sw_uam_parse_msg(argv, cyphertext, rc);
  if (rc == -1)
    return 0; /* Ignore parser errors */

  /* Optional serial number & stateid */
  if (argv[0] && argv[0][0] >= '0' && argv[0][0] <= '9') {
    memcpy(serial, argv[0], sizeof(serial));
    stateid = argv[1] ? atoi(argv[1]) : 0;
    for(argc=2; argc < SW_UAM_ARG_MAX; argc++) {
      argv[argc-2] = argv[argc];
    }
    argv[argc-1] = argv[argc-2] = NULL;
  } else {
    serial[0] = '\0';
  }

  if (!argv[0] || !argv[1] ) {
    sw_log("%s:%d empty UAM event and/or IPб %s  %s", __FILE__, __LINE__,argv[0],argv[1]);
    return 0; /* Ignore malformed events */
  }



  ip = ntohl(inet_addr(argv[1]));


  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d serial %s, stateid %d, event %s, session %s",
           __FILE__, __LINE__, serial[0] ? serial : "<none>",
           stateid, argv[0], sw_pretty_ip(ip));

  argc = 2; idx = 0;
  //pthread_rwlock_wrlock(mtx);

  switch(argv[0][0]) {
  case 'U':
    if (sw_session_verify_stateid(ip, &stateid)) {
        argv[0] = "ER";
        argv[argc++] = "stateid-verification-failure";
        break;
    }

    if (sw_gauge_new(ip,
                     argv[4] && *argv[4] ? atoll(argv[4]) : \
                     SW_GAUGE_MAX_OCTETS_IN,
                     argv[5] && *argv[5] ? atoll(argv[5]) : \
                     SW_GAUGE_MAX_OCTETS_OUT,
                     argv[6] && *argv[6] ? atoll(argv[6]) : \
                     SW_GAUGE_MAX_BPS_IN,
                     argv[7] && *argv[7] ? atoll(argv[7]) : \
                     SW_GAUGE_MAX_BPS_OUT,
                     argv[8] && *argv[8] ? atol(argv[8]) : \
                     SW_GAUGE_MAX_DURATION,
                     argv[9] && *argv[9] ? atol(argv[9]) : \
                     SW_GAUGE_MAX_IDLE) == -1 || \
        sw_shape_new(ip,
                     argv[6] && *argv[6] ? atoll(argv[6]) : \
                     SW_GAUGE_MAX_BPS_IN,
                     argv[7] && *argv[7] ? atoll(argv[7]) : \
                     SW_GAUGE_MAX_BPS_OUT) == -1 || \
        sw_session_start(ip,
                         argv[2] ? argv[2] : "",
                         argv[3] ? atoi(argv[3]) : 0,
                         argv[10] ? argv[10] : "",
                         argv[11] ? argv[11] : "") == -1)
    {
      argv[0] = "ER";
      argv[argc++] = "session-starting-failure";
      sw_gauge_del(ip,1);
      sw_shape_del(ip);
    } else {
      sw_session_acquire_stateid(ip, &stateid, 1);
      argv[0] = "OK";
    }
    break;
  case 'D':
    if (sw_session_verify_stateid(ip, &stateid)) {
        argv[0] = "ER";
        argv[argc++] = "stateid-verification-failure";
        break;
    }
    if (sw_session_stop(ip,
                        argv[2] ? argv[2] : "",
                        SW_SESSION_TERM_USER,
                        argv[3] ? argv[3] : "",
                        argv[4] ? argv[4] : "") == -1 ||
        sw_gauge_del(ip,1) == -1) {
      argv[0] = "ER";
      argv[argc++] = "session-stopping-failure";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 1);
      argv[0] = "OK";
    }
    break;
  case 'S':
    if (sw_session_status(ip, &state, &session_id, &filter_name,
                          &interim_interval, &context, NULL) == -1) {
      argv[0] = "ER";
      argv[argc++] = "getting-state-failure";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 0);
      argv[0] = "OK";
      argv[argc++] = state == SW_SESSION_ST_RELEASED ? "UP" : "DN";
      argv[argc++] = sprintf(nbufs[idx], "%lu", session_id) < 0 ?
        "" : nbufs[idx++];
      argv[argc++] = filter_name;
      argv[argc++] = sprintf(nbufs[idx], "%d", interim_interval) < 0 ?
        "" : nbufs[idx++];
      argv[argc++] = context;
    }
    break;
  case 'L':
    if (sw_gauge_limits(ip,
                        &octets_in, &octets_out,
                        &bps_in, &bps_out,
                        &duration, &idle,1) == -1) {
      argv[0] = "ER";
      argv[argc++] = "getting-limits-failure";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 0);
      argv[0] = "OK";
      argv[argc++] = sprintf(nbufs[idx], "%llu", octets_in) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", octets_out) < 0 ? 
        "":nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", bps_in) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", bps_out) < 0 ? 
        "":nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%u", duration) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%u", idle) < 0 ? 
        "" : nbufs[idx++];
    }
    break;
  case 'C':
    if (sw_gauge_current(ip,
                         &octets_in, &octets_out, 
                         NULL, NULL,
                         &bps_in, &bps_out,
                         &duration, &idle,1) == -1) {
      argv[0] = "ER";
      argv[argc++] = "getting-currents-failure";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 0);
      argv[0] = "OK";
      argv[argc++] = sprintf(nbufs[idx], "%llu", octets_in) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", octets_out) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", bps_in) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", bps_out) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%u", duration) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%d", idle) < 0 ? 
        "" : nbufs[idx++];
    }
    break;
  case 'P':
    if (sw_gauge_peak(ip, &bps_in, &bps_out,1) == -1) {
      argv[0] = "ER";
      argv[argc++] = "getting-peaks-failure";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 0);
      argv[0] = "OK";
      argv[argc++] = sprintf(nbufs[idx], "%llu", bps_in) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%llu", bps_out) < 0 ? 
        "" : nbufs[idx++];
    }
    break;
  case 'O': {
    uint32_t snat_ip;
    uint16_t snat_lport, snat_hport;

    if (!sw_nat_src_enabled_flag ||
        sw_nat_src_endpoint_ip_getpub(&snat_ip, ip) == -1 ||
        sw_nat_src_endpoint_udp_getpub(&snat_lport, &snat_hport, ip) == -1) {
      argv[0] = "ER";
      argv[argc++] = sw_nat_src_enabled_flag ? "getting-public-port-failure" : "snat-is-disabled";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 0);
      argv[0] = "OK";
      argv[argc++] = sprintf(nbufs[idx], "%s", sw_pretty_ip(snat_ip)) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%u", snat_lport) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%u", snat_hport) < 0 ? 
        "" : nbufs[idx++];
    }
    break;
  }
  case 'I': {
    uint32_t snat_ip;
    uint16_t snat_port;

    if (!argv[2]) {
      sw_log("%s:%d missing public port", __FILE__, __LINE__);
      argv[0] = "ER";
      argv[argc++] = "missing-public-port";
      break;
    }

    snat_ip = ip;
    snat_port = atoi(argv[2]);

    if (!sw_nat_src_enabled_flag ||
        sw_nat_src_endpoint_udp_getprv(&ip, snat_ip, snat_port) == -1) {
      argv[0] = "ER";
      argv[argc++] = sw_nat_src_enabled_flag ? "getting-private-port-failure" : "snat-is-disabled";
    } else {
      sw_session_acquire_stateid(ip, &stateid, 0);
      argv[0] = "OK";
      argv[argc++] = sprintf(nbufs[idx], "%s", 
                             sw_pretty_port(snat_port)) < 0 ? 
        "" : nbufs[idx++];
      argv[argc++] = sprintf(nbufs[idx], "%s", sw_pretty_ip(ip)) < 0 ? 
        "" : nbufs[idx++];
    }
    break;
  }
  case 'R': {
    time_t retention = argv[2] ? atoi(argv[2]) : 0;

    if (argv[2]) {
      if (sw_session_verify_stateid(ip, &stateid)) {
          argv[0] = "ER";
          argv[argc++] = "stateid-verification-failure";
          break;
      }
    }
 
    if (sw_session_retention(ip, &retention) == -1) {
      argv[0] = "ER";
      argv[argc++] = "retention-operation-failure";
    } else {
      sw_session_acquire_stateid(ip, &stateid, argv[2] ? 1 : 0);
      argv[0] = "OK";
      argv[argc++] = sprintf(nbufs[idx], "%lu", retention) < 0 ? 
        "" : nbufs[idx++];
    }
    break;
  }
  default:
    argv[0] = "ER";
    argv[argc++] = "uam-protocol-error";
    break;
  }

   //pthread_rwlock_unlock(mtx);

  argv[argc] = NULL; /* end-of-vector */

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d serial %s, stateid %d, resolution %s %s",
           __FILE__, __LINE__, serial[0] ? serial : "<none>", stateid, 
           argv[0], argv[1] ? argv[1] : "");

  if (!global_report_error_flag && argv[0][0] == 'E') {
    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d mute error report", __FILE__, __LINE__);
    return 0;
  }

  /* Insert serial at the zero position if serial was in request */
  if (serial[0]) {
    for(;argc >= 0;argc--) {
      argv[argc+2] = argv[argc];
    }
    argv[0] = serial;
    argv[1] = sprintf(nbufs[idx], "%lu", stateid) < 0 ?
                  "" : nbufs[idx++];
  }

  rc = sw_uam_build_msg(argv, &p);
  if (rc == -1)
    return 0; /* Ignore parser errors */

  rc = sendto(self->socket, p, rc, 0, (struct sockaddr *)&addr_, addr_size);
  if (rc == -1) {
    sw_log("%s:%d sendto() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d sent UAM msg: %d octets", __FILE__, __LINE__, rc);

  return 0;
}

sw_socket_handler_t uam_server = {
  "UAM-SERVER",
  0,
  0,
  0,
  0,
  0,
  &__create,
  NULL,
  &__destroy,
  &__recv,
  &__proc,
  NULL
};
