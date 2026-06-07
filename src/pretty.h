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
#ifndef __SW_PRETTY_H__
#define __SW_PRETTY_H__

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include "portab.h"

#define SW_PRETTY_BUFFERS_MAX  6400
#define SW_PRETTY_BUFFERS_SIZE 256 /* XXX no range control ahead */

#ifdef __cplusplus
extern "C" {
#endif

  extern char *sw_pretty_get_buffer PROTO(());
  extern char *sw_pretty_ip PROTO((uint32_t ip));
  extern char *sw_pretty_port PROTO((uint16_t port));
  extern char *sw_pretty_proto PROTO((uint16_t protocol));
  extern char *sw_pretty_flags PROTO((int flags));

#ifdef __cplusplus
}
#endif

#endif
