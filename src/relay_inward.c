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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <errno.h>
#include "sweetspot.h"
#include "if.h"
#include "pretty.h"
#include "relay_inward.h"
#include "tuple.h"
#include "gauge.h"
#include "nat_dst.h"
#include "nat_dst_endpoint.h"
#include "nat_src.h"
#include "frag.h"
#include "frag_tuple.h"
#include "filter.h"
#include "filter_match.h"
#include "shape.h"
#include "memgmt.h"
#include "cfg.h"
#include "log.h"
#include <netinet/in.h>
#include <pthread.h>
//#include "mTable.h"
#include "cache.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

#define DROP_SHAPE_SEND_FAILED    0
#define DROP_SHAPE_SCHEDULE_ERROR 1
#define DROP_SHAPE_DELAY_ERROR    2
#define PACKETS_IN                3
#define BYTES_IN                  4
#define DROP_SNAT_ASSIGN_FAILED   5
volatile static int global_stats_relay_in[6];



static int __create(sw_socket_handler_t *self) {
  CONST char *int_if, *ext_if;

  int_if = sw_config_get("inner-interface", NULL);
  if (!int_if) {
    sw_log("%s:%d inner-interface not configured", __FILE__, __LINE__);
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d opening inner iface %s for input",
           __FILE__, __LINE__, int_if);

  self->socket = sw_if_open_for_input(int_if, 10, &self->ifindex);
  if (self->socket == -1) {
    return -1;
  }


  return 0;
}


static int __cconf_out(sw_socket_handler_t *self, int interface, int if_idx) {
  CONST char *int_if, *ext_if;



  self->priv = malloc(sizeof(sw_relay_inward_t));
  if (!self->priv) {
    sw_log("%s:%d malloc() failed: %s", __FILE__, __LINE__, strerror(errno));
    close(self->socket);
    return -1;
  }
  #ifdef AF_PACK_SEND_IMP
  ((sw_relay_inward_t *)self->priv)->socket = interface;
  ((sw_relay_inward_t *)self->priv)->if_index = if_idx;
  #else
  ext_if = sw_config_get("outer-interface", NULL);
  if (!ext_if) {
    sw_log("%s:%d outer-interface not configured", __FILE__, __LINE__);
    close(self->socket);
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d opening outer iface %s for output",
           __FILE__, __LINE__, ext_if);
  ((sw_relay_inward_t *)self->priv)->socket = sw_if_open_for_output(ext_if);
  #endif
  if (((sw_relay_inward_t *)self->priv)->socket == -1) {
    close(self->socket);
    free(self->priv);
    return -1;
  }

  return 0;
}

static int __destroy(sw_socket_handler_t *self) {
  CONST char *int_if, *ext_if;

  int_if = sw_config_get("inner-interface", "N/A");

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d closing inner iface %s for input",
           __FILE__, __LINE__, int_if);

  if (self->socket != -1)
    sw_if_close(self->socket);

  ext_if = sw_config_get("outer-interface", "N/A");

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d closing outer iface %s for output",
           __FILE__, __LINE__, ext_if);

  if (((sw_relay_inward_t *)self->priv)->socket != -1) {
    sw_if_close(((sw_relay_inward_t *)self->priv)->socket);
  }

  if (self->priv) {
    free(self->priv);
    self->priv = NULL;
  }

  return 0;
}

static int __recv(sw_socket_handler_t *self, char * buf,struct sockaddr_in * addr, char * cyphertext ) {
   return sw_if_recv(self->socket, buf, 65536);
}

static int __proc(sw_socket_handler_t *self, char * buf, int length,struct sockaddr_in * addr, char * cyphertext) {
  sw_tuple_t *osrc, *src, *dst, *odst, *nat;
  sw_filter_rule_t *rule = NULL;
  sw_memgmt_buf_t *p_buf;
  char * packet = buf;
  int delaytime, shape_flag;
  sw_tuple_t *hsrc, *hdst;
  hsrc = hdst = NULL;
  CachVal * cv;

  //sw_log("%s:%d processing inward packet, %d", __FILE__, __LINE__, length);

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d processing inward packet", __FILE__, __LINE__);

//  if ((length = sw_if_recv(self->socket, packet, sizeof(packet))) == -1) {
//    return -1;
//  }

  if (length == 0) {
    return 0; /* Drop */
  }

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d read %d octets", __FILE__, __LINE__, length);

  if (sw_tuple_fetch(&src, &dst, SW_TUPLE_PROTO_DEFAULT, 
                     packet, length) == -1) {
    sw_tuple_free(src); sw_tuple_free(dst);

    return 0; /* Drop */
  }
  
  if (sw_debug_flags & SW_DEBUG_RELAY) 
    sw_log("%s:%d from %s to %s", __FILE__, __LINE__,
           sw_tuple_pretty(src), sw_tuple_pretty(dst));

  //pthread_rwlock_wrlock(mtx);
  // if (sw_frag_do(&hsrc, &hdst, src, dst) == -1) {
  //   sw_tuple_free(src); sw_tuple_free(dst);
  //   sw_tuple_free(hsrc); sw_tuple_free(hdst);
  //   return 0; /* Drop */
  // }


  nat = osrc = odst = NULL;

  //pthread_rwlock_rdlock(&global_filter_lock);
  spinlock_lock(self->sp);
  int fm_res = 0;
  uint32_t cache_key = sw_tuple_hash_simpl(hsrc ? hsrc : src) ^ sw_tuple_hash_simpl(hdst ? hdst : dst);
  //sw_log("%s:%d filter for relation found in cache %d , src:%d, dst:%d",
    //     __FILE__, __LINE__, cache_key, sw_tuple_hash_simpl(hsrc ? hsrc : src),sw_tuple_hash_simpl(hdst ? hdst : dst));
  if(get(self->cache,cache_key,&cv))
  {
    fm_res = cv->type;
    rule = cv->rule;
    if (sw_debug_flags & SW_DEBUG_RELAY)
      sw_log("%s:%d filter for relation found in cache %s->%s",
             __FILE__, __LINE__, sw_tuple_pretty(src),
             sw_tuple_pretty(dst));
  }
  else
  {
  fm_res = sw_filter_match(&rule, hsrc ? hsrc : src, hdst ? hdst : dst,
                               SW_FILTER_MATCH_INWARD);
    if(fm_res>=0)
    {
        CachVal tcv;
        tcv.rule = rule;
        tcv.type = fm_res;
        put(self->cache,cache_key,tcv);
    }
    if (sw_debug_flags & SW_DEBUG_RELAY)
      sw_log("%s:%d filter for relation not found in cache %s->%s",
             __FILE__, __LINE__, sw_tuple_pretty(src),
             sw_tuple_pretty(dst));
  }
  //sw_log("%s:%d inward mtx bloking 1 pos: %d", __FILE__, __LINE__, getBlockQty());


  //pthread_rwlock_unlock(mtx);
  switch(fm_res) {
  case 0:
    shape_flag = 0;
    if (sw_debug_flags & SW_DEBUG_RELAY)
      sw_log("%s:%d unmatched packet %s->%s passed filter",
             __FILE__, __LINE__, sw_tuple_pretty(src),
             sw_tuple_pretty(dst));
    break;
  case 1:
    if(!rule)
    {
         sw_log("%s:%d rule is NULL wrong filter setting %s->%s dropper", 
               __FILE__, __LINE__, sw_tuple_pretty(src),
               sw_tuple_pretty(dst));
        spinlock_unlock(self->sp);
        return 0;

    }	    
    shape_flag = rule->action&SW_FILTER_ACTION_SHAPE;
    switch(rule->action) {
    case SW_FILTER_ACTION_PASS:
    case SW_FILTER_ACTION_PASS|SW_FILTER_ACTION_SHAPE:
      if (sw_debug_flags & SW_DEBUG_RELAY)
        sw_log("%s:%d matched packet %s->%s passed filter%s", 
               __FILE__, __LINE__, sw_tuple_pretty(src),
               sw_tuple_pretty(dst),
               shape_flag ? " & shaped" : "");
      break;
    case SW_FILTER_ACTION_BLOCK:
      if (sw_debug_flags & SW_DEBUG_RELAY)
        sw_log("%s:%d matched packet %s->%s dropped filter", 
               __FILE__, __LINE__, sw_tuple_pretty(src),
               sw_tuple_pretty(dst));
      if(src)sw_tuple_free(src); if(dst)sw_tuple_free(dst);
            if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
     // pthread_rwlock_unlock(&global_filter_lock);
            spinlock_unlock(self->sp);
      return 0;
    case SW_FILTER_ACTION_DNAT:
    case SW_FILTER_ACTION_DNAT|SW_FILTER_ACTION_SHAPE:
      if (hdst ? sw_frag_tuple_clone(&nat, hdst): \
          sw_tuple_clone(&nat, dst) == -1) {
        if(src)sw_tuple_free(src); if(dst)sw_tuple_free(dst);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
              if(nat) sw_tuple_free(nat);
      // pthread_rwlock_unlock(&global_filter_lock);
              spinlock_unlock(self->sp);
        return -1;
      }
      if (sw_nat_dst_endpoint_assign(nat, htonl(rule->dnat.ip), 
                                     htons(rule->dnat.port)) == -1) {
        sw_tuple_free(src); sw_tuple_free(dst); sw_tuple_free(nat);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
      // pthread_rwlock_unlock(&global_filter_lock);
              spinlock_unlock(self->sp);
        return -1;
      }
     //pthread_rwlock_wrlock(mtx);
      if (sw_nat_dst_do(hsrc ? hsrc : src, hdst ? hdst : dst, nat) == -1) {
        sw_tuple_free(src); sw_tuple_free(dst); sw_tuple_free(nat);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
      // pthread_rwlock_unlock(&global_filter_lock);
              spinlock_unlock(self->sp);
        return 0; /* Drop */
      }
    // pthread_rwlock_unlock(mtx);
      if (sw_debug_flags & SW_DEBUG_RELAY)
        sw_log("%s:%d packet %s->%s DNAT'ed to %s%s", __FILE__, __LINE__,
               sw_tuple_pretty(src), sw_tuple_pretty(dst),
               sw_tuple_pretty(nat),
               shape_flag ? " & shaped" : "");
      odst = dst;
      hdst ? sw_frag_tuple_clone(&dst, nat) : sw_tuple_clone(&dst, nat);
      sw_tuple_free(nat);
      break; /* Re-dir and pass */
    case SW_FILTER_ACTION_SHAPE:
      if (sw_debug_flags & (SW_DEBUG_RELAY|SW_DEBUG_SHAPE))
        sw_log("%s:%d matched packet %s->%s shaped filter",
               __FILE__, __LINE__, sw_tuple_pretty(src),
               sw_tuple_pretty(dst));
      break;
    default:
      sw_log("%s:%d unexpected decision %d", __FILE__, __LINE__, rule->action);
      sw_tuple_free(src); sw_tuple_free(dst);
            if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
     //pthread_rwlock_unlock(&global_filter_lock);
            spinlock_unlock(self->sp);
      return -1;
    }
    break;
  default:
      if (sw_debug_flags & SW_DEBUG_RELAY)
        sw_log("%s:%d unmatched packet %s->%s dropped filter",
               __FILE__, __LINE__, sw_tuple_pretty(src),
               sw_tuple_pretty(dst));
    sw_tuple_free(src); sw_tuple_free(dst);
          if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
    //pthread_rwlock_unlock(&global_filter_lock);
          spinlock_unlock(self->sp);
    return 0; /* Drop */
  }
  spinlock_unlock(self->sp);
  //pthread_rwlock_unlock(&global_filter_lock);

  //releaseAllMtx();

  //pthread_rwlock_wrlock(mtx);
 
  if (sw_gauge_drop_stat(self->cBuf,hsrc ? hsrc : src, length, SW_GAUGE_DIR_IN) == -1) {
    sw_tuple_free(src); sw_tuple_free(dst);
          if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
    if (odst) sw_tuple_free(odst);

    return -1;
  }
  //pthread_rwlock_unlock(mtx);

  if (sw_nat_src_enabled_flag) {
    if (sw_nat_src_do(&nat, hsrc ? hsrc : src) == -1) {
    global_stats_relay_in[DROP_SNAT_ASSIGN_FAILED]++;
      sw_tuple_free(src); sw_tuple_free(dst);
            if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
      if (odst) sw_tuple_free(odst);
      if (nat) sw_tuple_free(nat);
      return 0; /* Drop */
    }


    osrc = src;
    hsrc ? sw_frag_tuple_clone(&src, nat) : sw_tuple_clone(&src, nat);
    if (nat) sw_tuple_free(nat);
  }
  //releaseAllMtx();



  if (sw_tuple_commit(packet, length, src, dst, osrc, odst) == -1) {
    sw_tuple_free(src); sw_tuple_free(dst);
          if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
    if (osrc) sw_tuple_free(osrc); if (odst) sw_tuple_free(odst);
    //sw_log("%s:%d inward mtx bloking 2 pos: %d", __FILE__, __LINE__, getBlockQty());

    return 0; /* Drop */
  }



/*  global_stats_relay_in[PACKETS_IN]++;
  global_stats_relay_in[BYTES_IN] += length; */

  if (shape_flag) {
    switch(delaytime = sw_shape_schedule(hsrc ? hsrc : osrc, length,
                                         rule->shape.rate, SW_SHAPE_IN, NULL)) {
      case 0: /* send packet immediately */
        if (sw_if_send(((sw_relay_inward_t *)self->priv)->socket,
                   packet, length,1,((sw_relay_inward_t *)self->priv)->if_index) == -1) {
          global_stats_relay_in[DROP_SHAPE_SEND_FAILED]++;
          break; /* Drop */
        }
        if (sw_debug_flags & (SW_DEBUG_RELAY | SW_DEBUG_SHAPE))
          sw_log("%s:%d %s->%s sent", __FILE__, __LINE__,
                  sw_tuple_pretty(src), sw_tuple_pretty(dst));
        break;
      case -1: /* error */
        global_stats_relay_in[DROP_SHAPE_SCHEDULE_ERROR]++;
        break; /* Drop */
      default: /* packet is placed into a queue */
        if (sw_memgmt_get_buf(&p_buf, length) == -1)
          break; /* Drop */
        memcpy(p_buf->buf, packet, length);

        if (sw_shape_delay(0, delaytime, p_buf, length) == -1) {
          global_stats_relay_in[DROP_SHAPE_DELAY_ERROR]++;
          sw_memgmt_free_buf(p_buf->idx);
        } else if (sw_debug_flags & SW_DEBUG_SHAPE)
            sw_log("%s:%d %s->%s placed into a queue %d", __FILE__, __LINE__,
                    sw_tuple_pretty(src), sw_tuple_pretty(dst), self->socket);
    }
  } else
  {

    sw_if_send(((sw_relay_inward_t *)self->priv)->socket, packet, length,1,((sw_relay_inward_t *)self->priv)->if_index);

  }  /* if sw_if_send failed => packet dropped */

  if (osrc) sw_tuple_free(osrc); if (odst) sw_tuple_free(odst);
  sw_tuple_free(src); sw_tuple_free(dst);
  if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);



  return 0;
}

static int __send(sw_socket_handler_t *self, char *packet, int length) {
  return sw_if_send(((sw_relay_inward_t *)self->priv)->socket, packet, length,1,((sw_relay_inward_t *)self->priv)->if_index);
}

int sw_relay_inward_stats_null() {
  global_stats_relay_in[DROP_SHAPE_SEND_FAILED] = 0;
  global_stats_relay_in[DROP_SHAPE_SCHEDULE_ERROR] = 0;
  global_stats_relay_in[DROP_SHAPE_DELAY_ERROR] = 0;
  global_stats_relay_in[PACKETS_IN] = 0;
  global_stats_relay_in[BYTES_IN] = 0;
  global_stats_relay_in[DROP_SNAT_ASSIGN_FAILED] = 0;
  return 0;
}

int sw_relay_inward_stats_print() {
  sw_log("%s:%d INWARD_drops: send %d, schedule %d, delay %d",
          __FILE__, __LINE__, global_stats_relay_in[DROP_SHAPE_SEND_FAILED],
          global_stats_relay_in[DROP_SHAPE_SCHEDULE_ERROR],
          global_stats_relay_in[DROP_SHAPE_DELAY_ERROR],
	  global_stats_relay_in[DROP_SNAT_ASSIGN_FAILED]);
/*  sw_log("%s:%d INWARD_stats: packets %d, bytes %d", __FILE__, __LINE__,
         global_stats_relay_in[PACKETS_IN], global_stats_relay_in[BYTES_IN]); */
  return 0;
}

sw_socket_handler_t relay_inward = {
  "INWARD",
  0,
  0,
  0,
  0,
  0 ,
  &__create,
  &__cconf_out,
  &__destroy,
  &__recv,
  &__proc,
  &__send,
  NULL
};
