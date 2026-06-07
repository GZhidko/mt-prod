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

#include "time_op.h"

#define DEGREE 1000000

int tv_add(struct timeval *res, struct timeval *tv1, struct timeval *tv2) {
  res->tv_sec = tv1->tv_sec + tv2->tv_sec;
  res->tv_usec = tv1->tv_usec + tv2->tv_usec;
  if (res->tv_usec >= DEGREE) {
    res->tv_sec += 1;
    res->tv_usec -= DEGREE;
  }
  return 0;
}

int tv_sub(struct timeval *res, struct timeval *tv1, struct timeval *tv2) {
  res->tv_sec = tv1->tv_sec - tv2->tv_sec;
  if (res->tv_sec < 0) {
    res->tv_sec = 0;
    res->tv_usec = 0;
    return 0;
  } 
  if (tv1->tv_usec >= tv2->tv_usec)
    res->tv_usec = tv1->tv_usec - tv2->tv_usec;
  else if (res->tv_sec > 0) { 
    res->tv_sec -= 1;
    res->tv_usec = tv1->tv_usec + DEGREE - tv2->tv_usec;
  } else res->tv_usec = 0;
  return 0;
}

int tv_cmp(struct timeval *tv1, struct timeval *tv2) {
  if (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec == tv2->tv_usec) return 0;
  if (tv1->tv_sec > tv2->tv_sec || \
     (tv1->tv_sec == tv2->tv_sec && tv1->tv_usec > tv2->tv_usec)) return 1;
  else return 2;
}

int tv2usec (struct timeval *tv) {
  int res;
  res = tv->tv_sec * DEGREE + tv->tv_usec;
  return res;
}
