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
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "cfg.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static sw_config_entry_t *global_config_entries = NULL;

int sw_config_load(CONST char *filename) {
  FILE *f = fopen(filename, "r");
  sw_config_entry_t *c;
  char b[4096], *l, *k, *v;

  if (f == NULL) {
    sw_log("%s:%d fopen() for %s failed: %s", __FILE__, __LINE__, 
           filename, strerror(errno));
    return -1;
  }

  while((l = fgets(b, sizeof(b)-1, f)) != NULL) {
    v = l;
    while(*v && *v != '\r' && *v != '\n') v++;
    *v = '\0';
    while(*l && isspace(*l)) l++;
    if (*l == '\0' || *l == '#')
      continue;

    /* Key */
    k = l;
    while(*l && !isspace(*l)) l++;
    if (*l == '\0') {
      sw_log("%s:%d missing config value %s", __FILE__, __LINE__, k);
      fclose(f);
      return -1;
    }
    *l = '\0'; l++;
    k = strdup(k);

    while(*l && isspace(*l)) l++;

    /* Value */
    v = strdup(l);
    
    c = malloc(sizeof(sw_config_entry_t));
    if (c == NULL) {
      sw_log("%s:%d malloc() failed", __FILE__, __LINE__);
      fclose(f);
      return -1;
    }
    c->key = k;
    c->value = v;
    c->next = global_config_entries;
    global_config_entries = c;
  }

  fclose(f);

  return 0;
}

CONST char *sw_config_get(char *key, char *def) {
  sw_config_entry_t *c;

  for(c=global_config_entries; c != NULL; c=c->next) {
    if(c->key!=NULL){
    if (strcasecmp(c->key, key))
      continue;
    return c->value;
    }
  }

  return def;
}

int sw_config_unload() {
  sw_config_entry_t *p, *n = global_config_entries;

  while(n) {
    p = n;
    n = n->next;
    free(p);
  }

  return 0;
}
