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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include "log.h"
#include <pthread.h>
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static char *global_logfile = NULL;
static char global_pid[10];

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int sw_log_init(char *filename) {
  if (global_logfile) {
    free(global_logfile);
    global_logfile = NULL;
  }
  global_logfile = strdup(filename);
  int test_pid = getpid();
  sprintf(global_pid, "[%d]", getpid());
  return sw_log("logging PID %s into %s", global_pid, global_logfile);
}

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
  int fd;

#ifdef __STDC__
  va_start(pvar, format);
#else
  va_start(pvar);
  format = va_arg(pvar, char *);
#endif
 pthread_mutex_lock(&log_mutex);
  strcpy(b, asctime(localtime(&now)));
  b[strlen(b)-1] = ' ';
  strcat(b, global_pid);

  vsnprintf(b+strlen(b), sizeof(b)-strlen(b), format, pvar);
  strncat(b, "\n", sizeof(b)-1);
  b[sizeof(b)-1] = '\0';
  va_end(pvar);

  fd = open(global_logfile, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
  if (fd == -1) {
    return -1;
  }

  if (write(fd, b, strlen(b)) != strlen(b)) {
    close(fd);
    return -1;
  }
  pthread_mutex_unlock(&log_mutex);
  close(fd);

  return 0;
}
