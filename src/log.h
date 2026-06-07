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
#ifndef __SW_LOG_H__
#define __SW_LOG_H__

#include "debug.h"
#include "portab.h"

#ifdef __cplusplus
extern "C" {
#endif
  extern int sw_log_init PROTO((char *filename));
  extern int sw_log PROTO((CONST char *format, ...));
#ifdef __cplusplus
}
#endif

#endif
