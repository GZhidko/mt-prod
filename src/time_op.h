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

#ifndef __SW_TIME_OP_H__
#define __SW_TIME_OP_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <sys/time.h>
#include "portab.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int tv_add PROTO((struct timeval *res, 
                         struct timeval *tv1, struct timeval *tv2));
extern int tv_sub PROTO((struct timeval *res,
                         struct timeval *tv1, struct timeval *tv2));
extern int tv_cmp PROTO((struct timeval *tv1, struct timeval *tv2));

#ifdef __cplusplus
}
#endif

#endif

