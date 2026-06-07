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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <time.h>
#include "log.h"
#include <pthread.h>
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif



static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int sw_log_init(char *filename) { return 0; }

#ifdef __STDC__
int sw_log(CONST char *format, ...) {
#else
int sw_log(va_alist) va_dcl {
#endif
#ifndef __STDC__
  char *format;
#endif
  va_list pvar;
  static char b[1024];
  time_t now = time(NULL);
  pthread_mutex_lock(&log_mutex);

#ifdef __STDC__
  va_start(pvar, format);
#else
  va_start(pvar);
  format = va_arg(pvar, char *);
#endif


  strcpy(b, asctime(localtime(&now)));
  b[strlen(b)-1] = ' ';



  vsnprintf(b+strlen(b), sizeof(b)-strlen(b), format, pvar);
  strncat(b, "\n", sizeof(b)-1);
  b[sizeof(b)-1] = '\0';
  va_end(pvar);



  fprintf(stderr, "%s", b);

  pthread_mutex_unlock(&log_mutex);

  return 0;
}
