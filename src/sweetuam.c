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
#include <string.h>
#include <getopt.h>
#include "uammsg.h"
#include "uamclt.h"
#include "session.h"
#include "cfg.h"
#include "sweetuam.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int usage() {
  fprintf(stderr, "Usage: sweetuam [ -hV ] [ -d <n> ] [ -c config-file ] [ -s serial ] [ -l state-id ] event ip [ arg [ arg [...] ] ]\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "    -h             this help message\n");
  fprintf(stderr, "    -V             software version information\n");
  fprintf(stderr, "    -d debug-what  [%s]\n", sw_debug_keywords());
  fprintf(stderr, "    -c filename    configuration file (default %s)\n",
          sw_uamclt_default_config_file);
  fprintf(stderr, "    -s [auto]      set or autogenerate request serial number (default off)\n");
  fprintf(stderr, "    -l state-id    set request state-id (default 0, e.g. no state-id)\n");
  fprintf(stderr, "Events:\n");
  fprintf(stderr, "  UP ip [ filter-name [ interim-interval [ max-octets-in [ max-octets-out [ max-bps-in [ max-bps-out [ max-duration [ max-idle [ session-context [ event-context ] ] ] ] ] ] ] ] ] ] ] ]\n");
  fprintf(stderr, "  -> {state-id} OK ip\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  DN ip [ filter-name [ session-context [ event-context ] ] ]\n");
  fprintf(stderr, "  -> {state-id} OK ip\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  ST ip\n");
  fprintf(stderr, "  -> {state-id} OK ip UP|DN session-id filter-name interim-interval session-context\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  LI ip\n");
  fprintf(stderr, "  -> {state-id} OK ip max-octets-in max-octets-out max-bps-in max-bps-out max-duration max-idle\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  CN ip\n");
  fprintf(stderr, "  -> {state-id} OK ip octets-in octets-out bps-in bps-out duration idle\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  PE ip\n");
  fprintf(stderr, "  -> {state-id} OK ip bps-in bps-out\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  OU ip\n");
  fprintf(stderr, "  -> {state-id} OK ip snat-ip low-snat-port high-snat-port\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  IN snat-ip snat-port\n");
  fprintf(stderr, "  -> {state-id} OK snat-ip snat-port ip\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  RT ip [ retention ]\n");
  fprintf(stderr, "  -> {state-id} OK ip retention\n");
  fprintf(stderr, "  -> {state-id} ER error-message\n");
  fprintf(stderr, "  Arguments:\n");
  fprintf(stderr, "    [snat-]ip             xxx.xxx.xxx.xxx\n");
  fprintf(stderr, "    session-id            0..UINT32\n");
  fprintf(stderr, "    filter-name           string, 0..64\n");
  fprintf(stderr, "    interim-interval      in seconds, 0..UINT32\n");
  fprintf(stderr, "    [max-]octets-in       0..UINT64\n");
  fprintf(stderr, "    [max-]octets-out      0..UINT64\n");
  fprintf(stderr, "    [max-]bps-in          0..UINT64\n");
  fprintf(stderr, "    [max-]bps-out         0..UINT64\n");
  fprintf(stderr, "    [max-]duration        in seconds, 0..UINT32\n");
  fprintf(stderr, "    [max-]idle            in seconds, 0..UINT32\n");
  fprintf(stderr, "    [low-|high-]snat-port 0..65535\n");
  fprintf(stderr, "    session-context       string\n");
  fprintf(stderr, "    event-context         string\n");
  fprintf(stderr, "    retention             in seconds, 0..UINT32\n");
  return 0;
}

int main(int argc, char **argv) {
  sw_uamclt_group_t *group;
  CONST char *config_filename = sw_uamclt_default_config_file;
  char *sw_argv[SW_UAM_ARG_MAX];
  char serial[SW_UAM_ARG_SIZE], stateid[SW_UAM_ARG_SIZE];
  int argind;
  char c;
  extern char *optarg;
  extern int optind;

  serial[0] = '\0';
  stateid[0] = '0'; stateid[1] = '\0';

  while ((c = getopt(argc, argv, "hVd:c:s:l:")) != EOF) {
    switch (c) {
    case 'h':
      usage();
      return 0;
    case 'V':
      fprintf(stderr, "sweetspot version %s\n", VERSION);
      return 0;
    case 'd':
      if (sw_debug_create(optarg) == -1) {
        return -1;
      }
      break;
    case 'c':
      config_filename = optarg;
      break;
    case 's':
      if (strcmp(optarg, "auto") == 0) {
        srandom(time(NULL));
        sprintf(serial, "%d", random());
      } else {
        memset(serial, 0, sizeof(serial));
        strncpy(serial, optarg, sizeof(serial)-1);
      }
      break;
    case 'l':
      memset(stateid, 0, sizeof(stateid));
      strncpy(stateid, optarg, sizeof(stateid)-1);
      break;
    default:
      fprintf(stderr, "Bad option %c\n", c);
      usage();
      return -1;
    }
  }

  if (sw_debug_flags & (SW_DEBUG_UAM | SW_DEBUG_BREATH))
    sw_log("%s:%d sweetuam starting...", __FILE__, __LINE__);

  if (sw_config_load(config_filename) < 0) {
    printf("ER ? client-config-load-failure\n");
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_UAM) 
    sw_log("%s:%d config %s loaded", __FILE__, __LINE__, config_filename);

  if (sw_uam_create_client(&group) == -1) {
    printf("ER ? client-initialization-failure\n");
    return -1;
  }

  if (serial[0]) {
    sw_argv[0] = serial;
    sw_argv[1] = stateid;
    argind = 2;
  } else {
    argind = 0;
  }
 
  for(;argv[optind];optind++,argind++) {
    if (argind >= SW_UAM_ARG_MAX - 1) {
      fprintf(stderr, "Too many arguments (>%d)\n", SW_UAM_ARG_MAX);
      return -1;
    }
    sw_argv[argind] = argv[optind];
  }
  if (argind < (serial[0] ? 3 : 2)) {
    fprintf(stderr, "Event type and IP required...\n");
    usage();
    return -1;
  }
  for(;argind<SW_UAM_ARG_MAX;argind++)
    sw_argv[argind] = NULL;

  if (sw_uam_send_msg(group, sw_argv) == -1) {
    printf("ER %s client-send-failure\n", sw_argv[1]);
    return -1;
  }

  if (sw_debug_flags & (SW_DEBUG_UAM | SW_DEBUG_BREATH)) 
    sw_log("%s:%d uam message issued", __FILE__, __LINE__);

  argind = 0;

  while(1) {
    if (sw_uam_recv_msg(group, sw_argv) == -1) {
      printf("%sER %s client-recv-failure\n", serial[0] ? "{?} " : "", sw_argv[serial[0] ? 2 : 1]);
      return -1;
    }

    if (sw_debug_flags & (SW_DEBUG_UAM | SW_DEBUG_BREATH))
      printf("uam message received\n");  

    if (serial[0]) {
      if (strcmp(serial, sw_argv[0]) != 0) {
        if (sw_debug_flags & (SW_DEBUG_UAM | SW_DEBUG_BREATH))
          printf("uam message serial mismatched %s vs %s\n", serial, sw_argv[0]);
        continue;
      } else {
        printf("{%s} ", sw_argv[1]);
        argind += 2;
        break;
      }
    } else {
      break;
    }
  }

  for(;sw_argv[argind];argind++)
    printf("%s ", *sw_argv[argind] ? sw_argv[argind] : "<EMPTY>");
  printf("\n");

  sw_uam_destroy_client(group);

  sw_config_unload();

  return 0;
}
