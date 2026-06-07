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
#ifndef __SW_UAMMSG_H__
#define __SW_UAMMSG_H__

#include "portab.h"

extern int global_debug_flag;

#define SW_UAM_ARG_MAX           32
#define SW_UAM_ARG_SIZE          32
#define SW_UAM_MSG_SIZE          512

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_uam_build_msg PROTO((char **args, char **cyphertext));
  extern int sw_uam_parse_msg PROTO((char **args, char *cyphertext, 
                                     int cyberlen));

#ifdef __cplusplus
}
#endif

#endif
