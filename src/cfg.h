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
#ifndef __SW_CONFIG_H__
#define __SW_CONFIG_H__

#include "portab.h"

typedef struct __sw_config_entry_t {
  char *key;
  char *value;
  struct __sw_config_entry_t *next ;
} sw_config_entry_t;

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_config_load PROTO((CONST char *filename));
  extern CONST char *sw_config_get PROTO((char *key, char *def));
  extern int sw_config_unload PROTO(());

#ifdef __cplusplus
}
#endif

#endif
