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
#include <errno.h>
#include "sweetspot.h"
#include "pretty.h"
#include "tuple_checksum.h"
#include "log.h"
#include <stdint.h>
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

#ifndef CHECHSUMM_FULL_CALC

int sw_tuple_checksum_decrement(uint16_t *partial_csum, uint16_t csum,
                                int initial_flag,
                                unsigned char *packet, int length) {
  int32_t x = csum, old;

  if (length % 2) {
    sw_log("%s:%d odd length %d", __FILE__, __LINE__, length);
    return -1;
  }

  if (initial_flag)
    x = ~x & 0xffff;
  while (length) {
    old=packet[0]*256 + packet[1];
    packet += 2;
    x -= old & 0xffff;
    if (x <= 0) {
      x--;
      x &= 0xffff;
    }
    length -= 2;
  }
  *partial_csum = (uint16_t)(x & 0xffff);
  return 0;
}

int sw_tuple_checksum_increment(uint16_t *csum, uint16_t partial_csum,
                                int final_flag,
                                unsigned char *packet, int length) {
  int32_t x = partial_csum, new;

  if (length % 2) {
    sw_log("%s:%d odd length %d", __FILE__, __LINE__, length);
    return -1;
  }

  while (length) {
    new = packet[0]*256+packet[1]; packet+=2;
    x += new & 0xffff;
    if (x & 0x10000) {
      x++;
      x &= 0xffff;
    }
    length -= 2;
  }
  if (final_flag)
    x = ~x & 0xffff;
  *csum = (uint16_t) x & 0xffff;
  return 0;
}
#else
int sw_tuple_checksum_decrement(uint16_t *partial_csum, uint16_t csum,
                                int initial_flag,
                                unsigned char *packet, int length) {
  int32_t x = csum, old;

  if (length % 2) {
    sw_log("%s:%d odd length %d", __FILE__, __LINE__, length);
    return -1;
  }

  if (initial_flag)
    x = ~x & 0xffff;
  while (length) {
    old=packet[0]*256 + packet[1];
    packet += 2;
    x -= old & 0xffff;
    if (x <= 0) {
      x--;
      x &= 0xffff;
    }
    length -= 2;
  }
  *partial_csum = (uint16_t)(x & 0xffff);
  return 0;
}

//int sw_tuple_checksum_increment(uint16_t *csum, uint16_t partial_csum,
//                                int final_flag,
//                                unsigned char *packet, int length) {
//  int32_t x = partial_csum, new;

//  /*if (length % 2) {
//    sw_log("%s:%d odd length %d", __FILE__, __LINE__, length);
//    return -1;
//  }*/

//  while (length>0) {
//    new = packet[0]*256+packet[1];
//    if (length == 1) {
//          new = packet[0];
//          //len -= 1;
//    }
//    packet+=2;

//    x += new & 0xffff;
//    if (x & 0x10000) {
//      x++;
//      x &= 0xffff;
//    }

//    length -= 2;
//  }
//  if (final_flag)
//    x = ~x & 0xffff;
//  *csum = (uint16_t) x & 0xffff;
//  return 0;
//}



int sw_tuple_checksum_increment(uint16_t *csum, uint16_t partial_csum,
                                int final_flag,
                                unsigned char *packet, int length) {
    uint32_t x = partial_csum;
    uint16_t new;

    while (length > 1) {
        new = (packet[0] << 8) | packet[1]; // Reverse order for little-endian
        packet += 2;

        x += new;
        if (x >> 16) {
            x = (x & 0xFFFF) + 1;
        }

        length -= 2;
    }

    if (length == 1) {
        new = packet[0] << 8;
        x += new;
        if (x >> 16) {
            x = (x & 0xFFFF) + 1;
        }
    }

    if (final_flag) {
        x = ~x & 0xFFFF;
    }

    *csum = (uint16_t)x & 0xffff;
    return 0;
}
#endif
