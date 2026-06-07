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
#ifndef __SW_TUPLE_CHECKSUM_H__
#define __SW_TUPLE_CHECKSUM_H__

#include "portab.h"

extern int global_debug_flag;

#ifdef __cplusplus
extern "C" {
#endif

  extern int sw_tuple_checksum_decrement PROTO((uint16_t *partial_csum,
                                                uint16_t csum,
                                                int initial_flag,
                                                unsigned char *packet,
                                                int length));
  extern int sw_tuple_checksum_increment PROTO((uint16_t *csum,
                                                uint16_t partial_csum,
                                                int final_flag,
                                                unsigned char *packet,
                                                int length));

#ifdef __cplusplus
}
#endif

#endif
