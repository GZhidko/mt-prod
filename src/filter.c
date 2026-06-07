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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "pretty.h"
#include "filter.h"
#include "cfg.h"
#include "log.h"
#include <pthread.h>
#include "cache.h"
#include "sweetspot.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static sw_filter_set_t *global_filter_set[SW_FILTER_SLOTS_MAX];
pthread_rwlock_t global_filter_lock;

static int sw_filter_rule_destroy (sw_filter_rule_t *rule) {
  if (rule) {
    sw_filter_rule_destroy(rule->next);
    if (sw_debug_flags & SW_DEBUG_FILTER )
      sw_log("%s:%d destroying rule [%d]", __FILE__, __LINE__, rule->num);
    free(rule);
  }
  return 0;
}

static sw_filter_set_t *sw_filter_load(char *filename,CONST char *dir_name) {
  sw_filter_set_t *filter; 
  sw_filter_rule_t *rs_in, *rs_out;
  struct stat stat_buf;
  char filter_name[1100], filter_text[16384];
  int fd, rc;

  rs_in = rs_out = NULL;

 
  if (strlen(dir_name) + 1 + strlen(filename) >= sizeof(filter_name) - 1) {
    sw_log("%s:%d %s/%s: name too long", __FILE__, __LINE__,
           dir_name, filename);
    return NULL;
  }

  sprintf(filter_name, "%s/%s", dir_name, filename);
  filter_name[sizeof(filter_name)-1] = '\0';

  fd = open(filter_name, O_RDONLY);
  if (fd == -1) {
    sw_log("%s:%d open() for %s failed: %s", __FILE__, __LINE__,
           filter_name, strerror(errno));
    return NULL;
  }

  rc = read(fd, &filter_text, sizeof(filter_text)-1);
  if (rc == -1 || rc == sizeof(filter_text)-1) {
    sw_log("%s:%d read() for %s failed: %s", __FILE__, __LINE__,
           filter_name, rc == -1 ? strerror(errno) : "filter too big");
    close(fd);
    return NULL;
  }

  filter_text[rc-1] = '\0';

  close(fd);

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d file \"%s\" read, %d chars", __FILE__, __LINE__,
           filename, rc);

  if (rc > sizeof(filter_text)-1) {
    sw_log("%s:%d filter text \"%s\" too large", __FILE__, __LINE__,
           filter_name);
    return NULL;
  }

  if (sw_filter_parse_ruleset(&rs_in, &rs_out, filter_text) == -1) {
    sw_filter_rule_destroy(rs_in);
    sw_filter_rule_destroy(rs_out);
    return NULL;
  }



  if (stat(filter_name, &stat_buf) == -1) {
    sw_log("%s:%d stat() failed: %s", __FILE__, __LINE__, strerror(errno));
    sw_filter_rule_destroy(rs_in);
    sw_filter_rule_destroy(rs_out);
    return NULL;
  }
  
  filter = malloc(sizeof(sw_filter_set_t));
  if (!filter) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    sw_filter_rule_destroy(rs_in);
    sw_filter_rule_destroy(rs_out);
    return NULL;
  }
  filter->name = strdup(filename);
  filter->mtime = stat_buf.st_mtime; 
  filter->ruleset_in = rs_in;
  filter->ruleset_out = rs_out;

  pthread_rwlock_init(&global_filter_lock,NULL);

  if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
    sw_log("%s:%d filter \"%s\" loaded (mtime %ld)",
           __FILE__, __LINE__, filter->name, filter->mtime);

  return filter;  
}

int sw_filter_create(CONST char *dir_name) {
  memset(global_filter_set, 0, sizeof(sw_filter_set_t *) * SW_FILTER_SLOTS_MAX);
  return sw_filter_reload(dir_name, time(NULL));
}
int sw_filter_reload(CONST char *dir_name, time_t now) {
  DIR *dirp;
  struct dirent *dp;
  struct stat stat_buf;
  int set_idx, file_idx;
  sw_filter_set_t *filter = NULL;
  static time_t filter_last_reload = 0;
  static int reload_interval = -1;
  sw_filter_fileinfo_t files[SW_FILTER_SLOTS_MAX];
  char new_filter_flag, destroy_filter_flag, filter_name[1100];

  if (reload_interval == -1) 
    reload_interval = atoi(sw_config_get("filter-reload-interval",
                                          SW_FILTER_RELOAD_INTERVAL));

  if (filter_last_reload > now ) return 0;
  filter_last_reload = now + reload_interval;
 
  memset(files, 0, sizeof(sw_filter_fileinfo_t) * SW_FILTER_SLOTS_MAX);

  if (!(dirp = opendir(dir_name))) {
    sw_log("%s:%d opendir() for %s failed: %s", __FILE__, __LINE__,
           dir_name, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
    sw_log("%s:%d dir %s opened", __FILE__, __LINE__, dir_name);

  file_idx = 0;
  while ((dp = readdir(dirp)) && file_idx < SW_FILTER_SLOTS_MAX) { 
    if (dp->d_ino == 0)
      continue;
    if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") ||
        (*dp->d_name && dp->d_name[strlen(dp->d_name)-1] == '~'))
      continue;
    if (strlen(dir_name) + 1 + strlen(dp->d_name) >= sizeof(filter_name) - 1) {
      sw_log("%s:%d %s/%s: name too long", __FILE__, __LINE__,
             dir_name, dp->d_name);
      continue;
    }
    sprintf(filter_name, "%s/%s", dir_name, dp->d_name);
    filter_name[sizeof(filter_name)-1] = '\0';
    if (stat(filter_name, &stat_buf) || S_ISDIR(stat_buf.st_mode))
      continue;

    files[file_idx].fullpath = strdup(filter_name);
    files[file_idx].filename = dp->d_name;
    files[file_idx].mtime = stat_buf.st_mtime;
    file_idx++;
  }
  if (dp) {
    if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
      sw_log("%s:%d max %d filters, you have more",
              __FILE__, __LINE__, SW_FILTER_SLOTS_MAX); 
      closedir(dirp);
      for (file_idx=0; file_idx<SW_FILTER_SLOTS_MAX; file_idx++) {
        if (files[file_idx].fullpath)
          free(files[file_idx].fullpath);
       else
         break;
      }
      return 0;
  }

  /* Checking filters to reload */
  
  for (file_idx=0; file_idx<SW_FILTER_SLOTS_MAX; file_idx++) {
    if (files[file_idx].fullpath) { 
      new_filter_flag = 1;
      for (set_idx=0; set_idx<SW_FILTER_SLOTS_MAX; set_idx++) {
        if (global_filter_set[set_idx] &&
            !strcmp(global_filter_set[set_idx]->name, files[file_idx].filename)) {
          new_filter_flag = 0;
          if (global_filter_set[set_idx]->mtime < files[file_idx].mtime) {
            if (!(filter = sw_filter_load(files[file_idx].filename, dir_name)))
              break; 
            //pthread_rwlock_wrlock(&global_filter_lock);
            for(int i = 0; i<process_qnty;i++){
                spinlock_lock(global_socket_handlers[i]->sp);
            }

            sw_filter_destroy(global_filter_set[set_idx]);
            global_filter_set[set_idx] = filter;

            for(int i = 0; i<process_qnty;i++){
                if(global_socket_handlers[i]->cache)
              resetCache(global_socket_handlers[i]->cache);
            }

            for(int i = 0; i<process_qnty;i++){
                spinlock_unlock(global_socket_handlers[i]->sp);
            }
            //sleep(1);
            //pthread_rwlock_unlock(&global_filter_lock);
            if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
              sw_log("%s:%d filter \"%s\" updated",
                     __FILE__, __LINE__, filter->name);
            break;
          }
        }
      } 
      /* load new filters */
      if (new_filter_flag && 
          !(filter = sw_filter_load(files[file_idx].filename,dir_name)))
        continue;
      set_idx=0;
      while (new_filter_flag && set_idx<SW_FILTER_SLOTS_MAX) {
        if (!global_filter_set[set_idx]) {
          new_filter_flag = 0;
          //pthread_rwlock_wrlock(&global_filter_lock);
          for(int i = 0; i<process_qnty;i++){
              spinlock_lock(global_socket_handlers[i]->sp);
          }
          global_filter_set[set_idx] = filter;
          //sleep(1);
          for(int i = 0; i<process_qnty;i++){
              if(global_socket_handlers[i]->cache)
            resetCache(global_socket_handlers[i]->cache);
          }
          //pthread_rwlock_unlock(&global_filter_lock);
          for(int i = 0; i<process_qnty;i++){
              spinlock_unlock(global_socket_handlers[i]->sp);
          }
          if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
            sw_log("%s:%d new filter \"%s\" (#%d) created",
                    __FILE__, __LINE__, filter->name, set_idx);
          if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
            sw_filter_debug(filter->name);
          break;
        } else {
          set_idx++;
        }
        if (set_idx == SW_FILTER_SLOTS_MAX) {
          if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
            sw_log("%s:%d some new filters will be created in %d sec",
                   __FILE__, __LINE__, SW_FILTER_RELOAD_INTERVAL);
          free(filter);
        }
      } 
    } else break;
  }
  /* Checking filters to destroy */
  for (set_idx=0; set_idx<SW_FILTER_SLOTS_MAX; set_idx++) { 
    if (global_filter_set[set_idx]) {
      destroy_filter_flag = 1;
      for (file_idx=0; file_idx<SW_FILTER_SLOTS_MAX; file_idx++) {
        if (files[file_idx].filename &&
            !strcmp(global_filter_set[set_idx]->name, files[file_idx].filename)) {
          destroy_filter_flag = 0;
          break;
        }
      }
      if (destroy_filter_flag) {
         //pthread_rwlock_wrlock(&global_filter_lock);
          for(int i = 0; i<process_qnty;i++){
              spinlock_lock(global_socket_handlers[i]->sp);
          }
        sw_filter_destroy(global_filter_set[set_idx]);
        global_filter_set[set_idx] = NULL;
        for(int i = 0; i<process_qnty;i++){
          if(global_socket_handlers[i]->cache)
          resetCache(global_socket_handlers[i]->cache);
        }
        //sleep(1);
         //pthread_rwlock_unlock(&global_filter_lock);
        for(int i = 0; i<process_qnty;i++){
            spinlock_unlock(global_socket_handlers[i]->sp);
        }
      }
    } 
  }

  closedir(dirp);
  for (file_idx=0; file_idx<SW_FILTER_SLOTS_MAX; file_idx++) {
     if (files[file_idx].fullpath)
       free(files[file_idx].fullpath);
     else
       break;
  }
  return 0;    
}

int sw_filter_destroy(sw_filter_set_t *filter) {
  if (filter) {
      sw_filter_rule_destroy(filter->ruleset_in);
      sw_filter_rule_destroy(filter->ruleset_out);
      if (sw_debug_flags & (SW_DEBUG_FILTER | SW_DEBUG_BREATH))
        sw_log("%s:%d old filter \"%s\" unloaded",
               __FILE__, __LINE__, filter->name);
      free(filter->name);
      free(filter);
  }
  return 0;
}

int sw_filter_get(sw_filter_rule_t **ruleset_in, 
                  sw_filter_rule_t **ruleset_out, int filter_id) {
  if (ruleset_in) 
    *ruleset_in = NULL;
  if (ruleset_out)
    *ruleset_out = NULL;

  if (filter_id == SW_FILTER_ID_NONE) {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d no filter", __FILE__, __LINE__);
    return 0;
  }
  if (filter_id < 0 || filter_id >= SW_FILTER_SLOTS_MAX ||
      !global_filter_set[filter_id]) {
    sw_log("%s:%d filter #%d not loaded", __FILE__, __LINE__, filter_id);
    return 0;
  }

  if (ruleset_in)
    *ruleset_in = global_filter_set[filter_id]->ruleset_in;
  if (ruleset_out)
    *ruleset_out = global_filter_set[filter_id]->ruleset_out;
  return 0;
}

int sw_filter_get_id(CONST char *name) {
  int idx;

  for(idx=0;idx<SW_FILTER_SLOTS_MAX;idx++) {
    if (global_filter_set[idx] && 
        !strcmp(global_filter_set[idx]->name, name)) {
      return idx;
    }
  }
  return -1; /* Not found */
}

int sw_filter_get_name(CONST char **name, int idx) {
  if (idx == SW_FILTER_ID_NONE) {
    *name = "";
    return 0;
  }
  if (idx < 0 || idx > SW_FILTER_SLOTS_MAX) {
    sw_log("%s:%d bad filter", __FILE__, __LINE__, idx);
    return -1;
  }
  if (!global_filter_set[idx]) {
    //sw_log("%s:%d non-existing filter", __FILE__, __LINE__, idx);
    return -1;
  }
  if (!global_filter_set[idx]->name) {
    sw_log("%s:%d non-existing filter", __FILE__, __LINE__, idx);
    return -1;
  }
  *name = global_filter_set[idx]->name;
  return 0;
}

static char *sw_filter_join_ports(uint16_t *ports, char port_cnt) {
  int idx, n;  
  char *buf = sw_pretty_get_buffer();
  for (idx = n = 0; idx < port_cnt; idx++) {
    if (n) { buf[n++] = ','; }
    n += sprintf(buf+n, "%d", ports[idx]);
  }
  return buf;
}

int sw_filter_debug(char *name) {
  sw_filter_rule_t *rs;
  int id, idx;

  id = sw_filter_get_id(name);
  if (id == -1) {
    sw_log("%s:%d no such filter %s", __FILE__, __LINE__, name);
    return -1;
  }

  if (!global_filter_set[id]->name) {
    sw_log("%s:%d filter %s (#%d) not loaded", 
           __FILE__, __LINE__, name, id);
    return 0;
  }

  sw_log("%s:%d filter %s (#%d) begins", __FILE__, __LINE__, name, id);

  for(idx=0; idx<2; idx++) {
    switch(idx) {
    case 0:
      rs = global_filter_set[id]->ruleset_in;
      sw_log("%s:%d inbound filter %s (#%d)", __FILE__, __LINE__, name, id);
      break;
    case 1:
      rs = global_filter_set[id]->ruleset_out;
      sw_log("%s:%d outbound filter %s (#%d)", __FILE__, __LINE__, name, id);
      break;
    }

    while(rs) {
      sw_log("%s:%d [%d] %s %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
             __FILE__, __LINE__,
             /* rule num */
             rs->num,
             /* action */
             rs->action & SW_FILTER_ACTION_PASS ? "pass" : 
             rs->action == SW_FILTER_ACTION_SHAPE ? "pass" :
             rs->action & SW_FILTER_ACTION_BLOCK ? "block" :
             rs->action & SW_FILTER_ACTION_DNAT ? "dnat" : "??",
             /* dir */
             rs->dir == SW_FILTER_DIR_IN ? "in" : 
             rs->dir == SW_FILTER_DIR_OUT ? "out" : "???",
             /* proto */
             rs->proto ? " proto " : "",
             rs->proto ? sw_pretty_proto(rs->proto) : "",
             /* source IP/mask */
             rs->src.ip ? " from " : "",
             rs->src.ip ? sw_pretty_ip(rs->src.ip) : "",
             rs->src.ip ? " mask " : "",
             rs->src.ip ? sw_pretty_ip(rs->src.mask) : "",
             /* source port comparation */
             rs->src.op ? " port " : "",
             rs->src.op ? ( rs->src.op == SW_FILTER_TARGET_EQ ? "eq " :
                            rs->src.op == SW_FILTER_TARGET_NE ? "ne " :
                            rs->src.op == SW_FILTER_TARGET_LT ? "lt " :
                            rs->src.op == SW_FILTER_TARGET_LE ? "le " :
                            rs->src.op == SW_FILTER_TARGET_GT ? "gt " :
                            rs->src.op == SW_FILTER_TARGET_GE ? "ge " :
                            rs->src.op == SW_FILTER_TARGET_RG ? "":"?? "):"",
             rs->src.op && rs->src.op != SW_FILTER_TARGET_RG ? \
                 sw_filter_join_ports(rs->src.ports, rs->src.port_cnt) : "",
             /* source port range */
             rs->src.op == SW_FILTER_TARGET_RG ? 
             sw_pretty_port(rs->src.port_lo) : "",
             rs->src.op == SW_FILTER_TARGET_RG ? ":" : "",
             rs->src.op == SW_FILTER_TARGET_RG ? 
             sw_pretty_port(rs->src.port_hi) : "",
             /* dest IP/mask */
             rs->dst.ip ? " to " : "",
             rs->dst.ip ? sw_pretty_ip(rs->dst.ip) : "",
             rs->dst.ip ? " mask " : "",
             rs->dst.ip ? sw_pretty_ip(rs->dst.mask) : "",
             /* dest port comparation */
             rs->dst.op ? " port " : "",
             rs->dst.op ? ( rs->dst.op == SW_FILTER_TARGET_EQ ? "eq " :
                            rs->dst.op == SW_FILTER_TARGET_NE ? "ne " :
                            rs->dst.op == SW_FILTER_TARGET_LT ? "lt " :
                            rs->dst.op == SW_FILTER_TARGET_LE ? "le " :
                            rs->dst.op == SW_FILTER_TARGET_GT ? "gt " :
                            rs->dst.op == SW_FILTER_TARGET_GE ? "ge " :
                            rs->dst.op == SW_FILTER_TARGET_RG ? "":"?? "):"",
             rs->dst.op && rs->dst.op != SW_FILTER_TARGET_RG ? \
                 sw_filter_join_ports(rs->dst.ports, rs->dst.port_cnt) : "",
             /* dest port range */
             rs->dst.op == SW_FILTER_TARGET_RG ?
             sw_pretty_port(rs->dst.port_lo) : "",
             rs->dst.op == SW_FILTER_TARGET_RG ? ":" : "",
             rs->dst.op == SW_FILTER_TARGET_RG ?
             sw_pretty_port(rs->dst.port_hi) : "",
             /* TCP flags */
             rs->flags.mask ? " flags " : "",
             rs->flags.mask ? sw_pretty_flags(rs->flags.set) : "",
             rs->flags.mask ? "/" : "",
             rs->flags.mask ? sw_pretty_flags(rs->flags.mask) : "",
             /* dnat */
             rs->action & SW_FILTER_ACTION_DNAT && 
             rs->dir == SW_FILTER_DIR_IN ? " dnat to " : "",
             rs->action & SW_FILTER_ACTION_DNAT &&
             rs->dir == SW_FILTER_DIR_IN ? sw_pretty_ip(rs->dnat.ip) : "",
             rs->action & SW_FILTER_ACTION_DNAT &&
             rs->dir == SW_FILTER_DIR_IN &&
             rs->dir == SW_FILTER_DIR_IN ? " port " : "",
             rs->action & SW_FILTER_ACTION_DNAT &&
             rs->dir == SW_FILTER_DIR_IN ? sw_pretty_port(rs->dnat.port) : "",
             /* rate */
             rs->action & SW_FILTER_ACTION_SHAPE ? " rate " : "",
             rs->action & SW_FILTER_ACTION_SHAPE ? sw_pretty_port(rs->shape.rate):"");
      rs = rs->next;
    }
  }
  
  sw_log("%s:%d filter %s (#%d) ends", __FILE__, __LINE__, name, id);

  return 0;
}
