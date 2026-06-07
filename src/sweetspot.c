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
#include "../config.h"
#endif
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>
#include "sweetspot.h"
#include "netset.h"
#include "relay_inward.h"
#include "relay_outward.h"
#include "session.h"
#include "gauge.h"
#include "uamsrv.h"
#include "nat_dst.h"
#include "shape.h"
#include "memgmt.h"
#include "time_op.h"
#include "frag.h"
#include "cfg.h"
#include "acct.h"
#include "acct_scope.h"
#include "acct_method.h"
#include "acct_method_detail.h"
#include "filter.h"
#include "lhstat.h"
#include "log.h"
#include "sh_queue.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#define _GNU_SOURCE
#include <sched.h>
//#include "mTable.h"
#include <if.h>
#include "nat_src.h"
#include "cache.h"
#include <unistd.h> // for sysconf
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif
#define MAX_PROC_CNT 1

#define MULTI_QUEUE


// =============================
// Watchdog configuration
// =============================
#define WATCHDOG_MAX_THREADS 1024
#define WATCHDOG_CHECK_INTERVAL_SEC 10
#define WATCHDOG_TIMEOUT_SEC 30


char inner_gw_mac[8] = {0,0,0,0,0,0,0,0};

char outer_gw_mac[8] = {0,0,0,0,0,0,0,0};


int global_alive_flag = 1;
int process_qnty = 0;





sw_socket_handler_t * r_in;
sw_socket_handler_t * r_out;



/* Socket handlers registration */
sw_socket_handler_t ** global_socket_handlers;
//= {
//  &r_in[0],
//  &r_in[1],
//  &r_in[2],
//  &r_in[3],
//    &r_in[4],
//    &r_in[5],

//  &r_out[0],
//  &r_out[1],
//  &r_out[2],
//  &r_out[3],
//    &r_out[4],
//    &r_out[5],

//  &uam_server,
//  NULL
//};

typedef struct {
    pthread_t tid;
    atomic_long last_ping;
    int active;
} watchdog_worker_t;

typedef struct __rcv_in {
    sw_socket_handler_t * swh;
    pthread_mutex_t shq_lock ;
    int id;
  }rcv_in;


static watchdog_worker_t g_watchdog[WATCHDOG_MAX_THREADS];
static atomic_int g_watchdog_enabled = 1;

typedef struct __uam_rcv_in {
    sw_socket_handler_t * swh;
    rcv_in * rcvi_p;
    char * filter_dir;
    int id;
  }uam_rcv_in;

typedef struct __maintenance_in {
    char * filter_dir;
    int id;
  }maintenance_in;


static inline void watchdog_register(pthread_t tid, int idx) {
    if (idx < 0 || idx >= WATCHDOG_MAX_THREADS) return;
    g_watchdog[idx].tid = tid;
    g_watchdog[idx].active = 1;
    atomic_store(&g_watchdog[idx].last_ping, time(NULL));
}

static inline void watchdog_ping(int idx) {
    if (idx < 0 || idx >= WATCHDOG_MAX_THREADS) return;
    if (g_watchdog[idx].active)
    {
	//sw_log("ping id:%d \n",idx); 
        atomic_store(&g_watchdog[idx].last_ping, time(NULL));
    }
}

/*static inline void watchdog_run_script_and_kill(const char *script) {
    pid_t pid = fork();
    if (pid == 0) {
        char *argv_exec[] = {"python3", (char *)script, NULL};
        execvp("python3", argv_exec);
        _exit(127);
    }
    kill(getpid(), SIGKILL);
}*/
static inline void watchdog_run_script_and_kill() {
    pid_t pid = fork();
    if (pid == 0) {
        // In child process: execute the watchdog restart script
        char *argv_exec[] = {"/usr/local/bin/sweet_watchdog.sh", NULL};
        execvp(argv_exec[0], argv_exec);
        perror("execvp failed");
        sw_log("%s:%d [watchdog] execvp failed: %s", __FILE__, __LINE__, strerror(errno));
        _exit(127); // if execvp fails
    }

    // Parent process: wait briefly to allow the script to start, then terminate
    sleep(1);
    kill(getpid(), SIGKILL);
}



#ifndef USE_SELCT_FOR_UAM

void *uam_recv_thr(void * rcv_i) {
    uam_rcv_in * rcv_ = (uam_rcv_in*)rcv_i;

    struct timespec tw = {0,1000};
    //Структура, в которую будет помещен остаток времени
    //задержки, если задержка будет прервана досрочно
    struct timespec tr;
    //Вывод сообщения о приостановки работы

    char * buf = malloc(65536);
    int length;
    struct sockaddr_in addr;
    char cyphertext[SW_UAM_MSG_SIZE];
//sw_log("start worker id:%d",rcv_->id);

    pid_t pid = syscall(SYS_gettid);

    while(global_alive_flag){
//    sleep(311000);
	watchdog_ping(rcv_->id);
	
          length = rcv_->swh->recv(rcv_->swh,buf,&addr,cyphertext);
          if(length > 0)
          {


            if(rcv_->swh->proc(rcv_->swh,buf,length,&addr, cyphertext) == -1)
            sw_log("%s:%d %s recv handler failed", __FILE__, __LINE__,
                 rcv_->swh->name);
            //releaseAllMtx();


          //Приостановка работы
          }
          else if(length == 0)
          {
              nanosleep (&tw, &tr);
          }
          else
          {

              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                          // Modify source IP and src port
                  nanosleep (&tw, &tr);

               } else {
                  sw_log("%s:%d %s UAM recv failed : %s", __FILE__, __LINE__,
                         rcv_->swh->name, strerror(errno));
                  global_alive_flag = 0;
                  break;
	               }

	          }
	    }

}
#else


void *uam_recv_thr(void *rcv_i) {
    uam_rcv_in *rcv_ = (uam_rcv_in *)rcv_i;
    time_t now, tick, stats_tick, stats_int;

    struct timespec tw = {0, 1000};
    struct timespec tr;
    char *buf = malloc(65536);
    int length;
    struct sockaddr_in addr;
    char cyphertext[SW_UAM_MSG_SIZE];

    fd_set readfds;
    int maxfd;
    int retval;

    pid_t pid = syscall(SYS_gettid);
    stats_int = atoi(sw_config_get("stats-print-interval", SW_SWEETSPOT_STATS_INT));

    tick = stats_tick = 0;

    while (global_alive_flag) {
	watchdog_ping(rcv_->id);i
	sleep(200);
        FD_ZERO(&readfds);
        FD_SET(rcv_->swh->socket, &readfds); // Add the socket to the set

        maxfd = rcv_->swh->socket + 1;

        // Set timeout to zero for non-blocking check
        struct timeval timeout = {0, 0};

        retval = select(maxfd, &readfds, NULL, NULL, &timeout);

        if (retval == -1) {
            sw_log("%s:%d %s UAM recv failed : %s", __FILE__, __LINE__,
                   rcv_->swh->name, strerror(errno));
            global_alive_flag = 0;
            break;
        } else if (retval == 0) {
            // No data, continue or perform other tasks
        } else {
            // Data is ready to be read
            if (FD_ISSET(rcv_->swh->socket, &readfds)) {
                length = rcv_->swh->recv(rcv_->swh, buf, &addr, cyphertext);
                if (length > 0) {
                    if (rcv_->swh->proc(rcv_->swh, buf, length, &addr, cyphertext) == -1)
                        sw_log("%s:%d %s recv handler failed", __FILE__, __LINE__,
                               rcv_->swh->name);
                } else if (length < 0) {
                    sw_log("%s:%d %s UAM recv failed : %s", __FILE__, __LINE__,
                           rcv_->swh->name, strerror(errno));
                    global_alive_flag = 0;
                    break;
                }
            }
        }

        now = time(NULL);
        #define __SW_TICK_INTERVAL   5

        if (tick < now) {
            tick = now + __SW_TICK_INTERVAL;
            sw_lhstat_do(now);
            sw_gauge_expire();
            sw_session_expire();
            sw_acct_commit(0);
        }

        if (stats_tick < now) {
            stats_tick = now + stats_int;
            sw_shape_stats_print();
            sw_shape_stats_null();
            sw_relay_inward_stats_print();
            sw_relay_inward_stats_null();
            sw_relay_outward_stats_print();
            sw_relay_outward_stats_null();
            sw_memgmt_stats_print();
        }
    }

    // Clean up resources
    free(buf);

    // Additional cleanup code if needed

    pthread_exit(NULL);
}

#endif

void *maintenance_thr(void *maintenance_i) {
    maintenance_in *maintenance = (maintenance_in *)maintenance_i;
    time_t now, tick, stats_tick, stats_int;

    stats_int = atoi(sw_config_get("stats-print-interval", SW_SWEETSPOT_STATS_INT));
    tick = stats_tick = 0;

    while (global_alive_flag) {
        sleep(1);
        watchdog_ping(maintenance->id);
        now = time(NULL);

#define __SW_TICK_INTERVAL 1
        if (tick < now) {
            tick = now + __SW_TICK_INTERVAL;
            sw_lhstat_do(now);
            sw_gauge_expire();
            sw_session_expire();
            sw_acct_commit(0);
            sw_filter_reload(maintenance->filter_dir, now);
        }

        if (stats_tick < now) {
            stats_tick = now + stats_int;
            sw_shape_stats_print();
            sw_shape_stats_null();
            sw_relay_inward_stats_print();
            sw_relay_inward_stats_null();
            sw_relay_outward_stats_print();
            sw_relay_outward_stats_null();
            sw_memgmt_stats_print();
        }
    }

    return NULL;
}


/*void *recv_thr(void * rcv_i) {
    rcv_in * rcv_ = (rcv_in*)rcv_i;
   // struct timespec ts = {5,0};
    struct timeval tw = {0,1000};

    fd_set fds_;
    //Структура, в которую будет помещен остаток времени
    //задержки, если задержка будет прервана досрочно

    //struct timespec tr;
    char * buf = malloc(65536);
    int length = 0;
    pid_t pid = syscall(SYS_gettid);
    rcv_->swh->cache = malloc(sizeof(StaticCache));
    rcv_->swh->cBuf = malloc(sizeof(CircularBufferGauge));
    initCache(rcv_->swh->cache);
    init_buffer(rcv_->swh->cBuf);
    gauge_buffer_register(rcv_->swh->cBuf);


    //registerPerr();

    for(;global_alive_flag;){

      FD_ZERO(&fds_);
      FD_SET(rcv_->swh->socket, &fds_);


      int retval = select(rcv_->swh->socket+1, &fds_, NULL, NULL, NULL);
      if(retval)
      {

          length = 0;


         // memset(buf,0x0,65536);
          length = rcv_->swh->recv(rcv_->swh,buf,NULL,NULL);


          if(length==0) {

              //sw_log("%s:%d %s EOF", __FILE__, __LINE__,
                //     rcv_->swh->name);
              continue;

          }
          else if(length < 0){
              sw_log("%s:%d %s recv failed", __FILE__, __LINE__,
                     rcv_->swh->name);
              global_alive_flag = 0;
              break;
          }


          if(rcv_->swh->proc(rcv_->swh,buf,length,0, 0) == -1){
              sw_log("%s:%d %s recv handler failed", __FILE__, __LINE__,
                     rcv_->swh->name);
              global_alive_flag = 0;
              break;
          }
          //releaseAllMtx();

      }
      else if(retval == -1){
           sw_log("%s:%d select() failed: %s",
                         __FILE__, __LINE__, strerror(errno));
           global_alive_flag = 0;
           break;

      }
      else
      {

          sw_log("%s:%d select() timeout: %s",
                        __FILE__, __LINE__, strerror(errno));

      }


    }

}
*/

void *recv_thr(void * rcv_i) {
    rcv_in * rcv_ = (rcv_in*)rcv_i;
   // struct timespec ts = {5,0};
//    struct timeval tw = {5,0};

    fd_set fds_;
    //С���к���а, в ко�о��� б�де� поме�ен о��а�ок в�емени
    //заде�жки, е�ли заде�жка б�де� п�е�вана до��о�но

    //struct timespec tr;
    char * buf = malloc(65536);
    int length = 0;
    pid_t pid = syscall(SYS_gettid);
    rcv_->swh->cache = malloc(sizeof(StaticCache));
    rcv_->swh->cBuf = malloc(sizeof(CircularBufferGauge));
    initCache(rcv_->swh->cache);
    init_buffer(rcv_->swh->cBuf);
    gauge_buffer_register(rcv_->swh->cBuf);
//    sw_log("start worker id:%d",rcv_->id);

    //registerPerr();

    for(;global_alive_flag;){
    watchdog_ping(rcv_->id);
      FD_ZERO(&fds_);
      FD_SET(rcv_->swh->socket, &fds_);
      struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 }; // 1 ms
      int retval = select(rcv_->swh->socket+1, &fds_, NULL, NULL, &timeout);
      if(retval > 0) {
          while (1) {
          watchdog_ping(rcv_->id);
              length = rcv_->swh->recv(rcv_->swh, buf, NULL, NULL);
              if (length < 0) {
                  if (errno == EWOULDBLOCK || errno == EAGAIN) {
                      // No more data available, break the loop
                      break;
                  } else {
                      // Error occurred, handle it
                      sw_log("%s:%d %s recv failed: %s", __FILE__, __LINE__,
                             rcv_->swh->name, strerror(errno));
                      global_alive_flag = 0;
                      break;
                  }
              } else if (length == 0) {
                  // Peer has closed the connection
                  //sw_log("%s:%d %s EOF", __FILE__, __LINE__,
                    //     rcv_->swh->name);
                  break;
              } else {
                  // Process the received data
                  if(rcv_->swh->proc(rcv_->swh, buf, length, 0, 0) == -1){
                      sw_log("%s:%d %s recv handler failed", __FILE__, __LINE__,
                             rcv_->swh->name);
                      global_alive_flag = 0;
                      break;
                  }
                  break;
              }
          }
      } else if (retval == 0) {
          // Timeout occurred
          watchdog_ping(rcv_->id);
//          sw_log("%s:%d select() timeout idx:%d", __FILE__, __LINE__,rcv_->id);
      } else {
          // Error occurred in select
          sw_log("%s:%d select() failed: %s",
                 __FILE__, __LINE__, strerror(errno));
          global_alive_flag = 0;
          break;
      }


    }

}



static void sig_hdl(int sig) {
  switch(sig) {
  case SIGHUP:
  case SIGINT:
  case SIGQUIT:
  case SIGTERM:
    sw_log("%s:%d signal %d, shutting down", __FILE__, __LINE__, sig);
    global_alive_flag = 0;
    break;
  default:
    sw_log("%s:%d signal %d, exiting", __FILE__, __LINE__, sig);
    exit(-1);
  }
}

static int usage() {
  fprintf(stderr, "Usage: sweetspot [ -hVf ] [ -d debug-what ] [ -c config-file ] [ -l log-file ]\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -h              this help message\n");
  fprintf(stderr, "  -V              software version information\n");
  fprintf(stderr, "  -f              stay foreground\n");
  fprintf(stderr, "  -d debug-what   [%s]\n", sw_debug_keywords());
  fprintf(stderr, "  -c filename     configuration file (default %s)\n",
          SW_SWEETSPOT_CONFIG_FILE);
  fprintf(stderr, "  -l filename     log file (default %s)\n",
          SW_SWEETSPOT_LOG_FILE);
  return 0;
}

void *run(void *handler_id) {
  int hid;
  struct timespec s;

  hid = *(int *)handler_id;
  s.tv_sec = 0;
  s.tv_nsec = SW_SHAPE_QUEUE_PROCESS_FRQ * 1000;
  //registerPerr();
  while (global_alive_flag) {
    nanosleep(&s, NULL);
    sw_shape_queue_process(global_socket_handlers[hid], hid);
    //releaseAllMtx();
  }

}

int main(int argc, char **argv) {
  fd_set fds;
  struct timeval tout, curr_time, shape_tick, shape_int;
  int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  int maxfd, idx, p_len;
  char *config_filename = SW_SWEETSPOT_CONFIG_FILE;
  char *log_filename = SW_SWEETSPOT_LOG_FILE;
  char *debug_keywords = NULL;
  int c, foreground_flag = 0;
  pthread_t run_thread1[512], run_thread2[512], rcv_thread[512], maintenance_thread;
  int arg_th1, arg_th2;
  extern char *optarg;
  CONST char *int_gw_ip, *ext_gw_ip, *int_iface, *ext_iface;
  const char *watchdog_script = sw_config_get("watchdog-script", "/opt/sweetspot/restart.py");


  if (pthread_rwlock_init(&global_filter_lock, NULL) != 0) {
        // Error handling: Initialization failed
        perror("pthread_rwlock_init");
        return 0;
    }


  uam_rcv_in unrv;
  maintenance_in maintenance;


  

  signal(SIGHUP, sig_hdl);
  signal(SIGINT, sig_hdl);
  signal(SIGQUIT, sig_hdl);
  signal(SIGTERM, sig_hdl);

  while ((c = getopt(argc, argv, "hVfd:c:l:")) != -1) {
    switch (c) {
    case 'h':
      usage();
      return 0;
    case 'V':
      fprintf(stderr, "sweetspot version %s\n", VERSION);
      return 0;
    case 'f':
      foreground_flag = 1;
      break;
    case 'd':
      debug_keywords = optarg;
      break;
    case 'c':
      config_filename = optarg;
      break;
    case 'l':
      log_filename = optarg;
      break;
    default:
      fprintf(stderr, "Bad option %c\n", c);
      usage();
      return -1;
    }
  }

  if (!foreground_flag) {
    int pid;

    if ((pid = fork()) == -1) {
      fprintf(stderr, "fork() failed: %s\n", strerror(errno));
      return -1;
    }
    
    if (pid != 0) {
      _exit(0);
    }

    fflush(stdin); fflush(stdout); fflush(stderr);
    setbuf(stdin,NULL); setbuf(stdout,NULL); setbuf(stderr,NULL);
    close(0); close(1); close(2);
    open("/dev/null",O_RDWR);
    dup(0); dup(0);
#ifdef HAVE_SETSID
    setsid();
#else
#ifdef HAVE_SETPGRP
#ifdef SETPGRP_VOID
    setpgrp();
#else
    setpgrp(0,0);
#endif
#endif
#endif
  }
  int test_pid = getpid();
  //printf("pid:%d",test_pid);

  if (sw_log_init(log_filename) == -1) {
    fprintf(stderr, "sw_log_init() failed for %s\n", log_filename);
    usage();
    return -1;
  }

  if (debug_keywords) {
    if (sw_debug_create(debug_keywords) == -1) {
      fprintf(stderr, "sw_debug_create() failed for %s\n", debug_keywords);
      return -1;
    }
  }

  if (sw_config_load(config_filename) < 0)
    return -1;

  if (sw_debug_flags & (SW_DEBUG_CORE | SW_DEBUG_BREATH))
    sw_log("%s:%d sweetspot version %s starting", __FILE__, __LINE__, VERSION);

  if (sw_debug_flags & SW_DEBUG_CORE)
    sw_log("%s:%d config %s loaded", __FILE__, __LINE__, config_filename);



  process_qnty = atoi(sw_config_get("thread_qnty", MAX_PROC_RCV_CNT));
  if (process_qnty == -1 || process_qnty % 2 != 0) {
      sw_log("%s:%d thread_qnty parameter is incorrect or thread_qnty mod 2 > 0", __FILE__, __LINE__);
            return -1;
  }
  int uam_thread_qnty = atoi(sw_config_get("uam-thread-qnty", "1"));
  if (uam_thread_qnty < 1) {
      sw_log("%s:%d uam-thread-qnty parameter is incorrect", __FILE__, __LINE__);
      return -1;
  }
  if (process_qnty + uam_thread_qnty + 1 >= WATCHDOG_MAX_THREADS) {
      sw_log("%s:%d total worker count exceeds watchdog capacity", __FILE__, __LINE__);
      return -1;
  }

  uam_rcv_in *uam_rcv_arr = malloc(sizeof(uam_rcv_in) * uam_thread_qnty);
  pthread_t *uam_rcv_threads = malloc(sizeof(pthread_t) * uam_thread_qnty);
  if (!uam_rcv_arr || !uam_rcv_threads) {
      sw_log("%s:%d malloc() failed for uam thread arrays", __FILE__, __LINE__);
      free(uam_rcv_arr);
      free(uam_rcv_threads);
      return -1;
  }

  r_in = malloc(sizeof(sw_socket_handler_t) * process_qnty/2);
  r_out = malloc(sizeof(sw_socket_handler_t) * process_qnty/2);
  global_socket_handlers = malloc(sizeof(sw_socket_handler_t*) * (process_qnty + 2));

  for(int i = 0; i<process_qnty/2;i++){
    r_in[i] = relay_inward;
    r_out[i] = relay_outward;
        r_in[i].sp = malloc(sizeof(spinlock));
    r_in[i].sp->locked = 0;
    r_out[i].sp = malloc(sizeof(spinlock));
    r_out[i].sp->locked = 0;
    global_socket_handlers[i] = &r_in[i];
    global_socket_handlers[i+process_qnty/2] = &r_out[i];
  }
  global_socket_handlers[process_qnty+1] = NULL;

  global_socket_handlers[process_qnty] = &uam_server;



  if (sw_lhstat_create() == -1)
    return -1;

  if (sw_filter_create(sw_config_get("filter-dir", SW_FILTER_DIR)) == -1) {
    return -1;
  }

  unrv.filter_dir = sw_config_get("filter-dir", SW_FILTER_DIR);

  if (sw_session_create() == -1) {
    return -1;
  }

  if (sw_gauge_create() < 0)
    return -1;

  if (sw_nat_dst_create() == -1)
    return -1;

  if (sw_frag_create() == -1)
    return -1;
#ifdef AF_PACK_SEND_IMP
  int_gw_ip = sw_config_get("inner-gw-ip", NULL);
  if (!int_gw_ip) {
    sw_log("%s:%d inner-gw-mac not configured", __FILE__, __LINE__);
    return -1;
  }

  int_iface = sw_config_get("inner-interface", NULL);
  if (!int_iface) {
    sw_log("%s:%d inner-interface not configured", __FILE__, __LINE__);
    return -1;
  }

  if(getMACFromIP(int_gw_ip,int_iface ,  inner_gw_mac)==-1)
  {
     return -1;
  };
  char mac_address[18];
  snprintf(mac_address, 18, "%hhx:%hhx:%hhx:%02x:%02x:%02x",
           (unsigned char)inner_gw_mac[0],
           (unsigned char)inner_gw_mac[1],
           (unsigned char)inner_gw_mac[2],
           (unsigned char)inner_gw_mac[3],
           (unsigned char)inner_gw_mac[4],
           (unsigned char)inner_gw_mac[5]);
  sw_log("%s:%d inner-gw-mac:%s configured", __FILE__, __LINE__, mac_address);

  ext_gw_ip = sw_config_get("outer-gw-ip", NULL);
  if (!ext_gw_ip) {
    sw_log("%s:%d outer-gw-mac not configured", __FILE__, __LINE__);
    return -1;
  }

  ext_iface = sw_config_get("outer-interface", NULL);
  if (!ext_iface) {
    sw_log("%s:%d outer-interface not configured", __FILE__, __LINE__);
    return -1;
  }

  if(getMACFromIP(ext_gw_ip,ext_iface, outer_gw_mac)==-1)
  {
     return -1;
  };
  snprintf(mac_address, 18, "%hhx:%hhx:%hhx:%02x:%02x:%02x",
           (unsigned char)outer_gw_mac[0],
           (unsigned char)outer_gw_mac[1],
           (unsigned char)outer_gw_mac[2],
           (unsigned char)outer_gw_mac[3],
           (unsigned char)outer_gw_mac[4],
           (unsigned char)outer_gw_mac[5]);
  sw_log("%s:%d outer-gw-mac:%s configured", __FILE__, __LINE__, mac_address);

  for(idx=0;global_socket_handlers[idx];idx++) {
    if (!global_socket_handlers[idx]->create)
      continue;
    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d creating %s", __FILE__, __LINE__,
             global_socket_handlers[idx]->name);
    if (global_socket_handlers[idx]->create(global_socket_handlers[idx]) == -1)
      return -1;
    if (!global_socket_handlers[idx]->cconf_out)
      continue;
    if (global_socket_handlers[(idx+process_qnty/2)%process_qnty]->
            cconf_out(global_socket_handlers[(idx+process_qnty/2)%process_qnty],global_socket_handlers[idx]->socket, global_socket_handlers[idx]->ifindex) == -1)
      return -1;
    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d created %s, socket %d",
             __FILE__, __LINE__,
             global_socket_handlers[idx]->name,
             global_socket_handlers[idx]->socket);
  }
#else
  for(idx=0;global_socket_handlers[idx];idx++) {
    if (!global_socket_handlers[idx]->create)
      continue;
    if (sw_debug_flags & SW_DEBUG_CORE) 
      sw_log("%s:%d creating %s", __FILE__, __LINE__,
             global_socket_handlers[idx]->name);
    if (global_socket_handlers[idx]->create(global_socket_handlers[idx]) == -1)
      return -1;
    if (!global_socket_handlers[idx]->cconf_out)
      continue;
    if (global_socket_handlers[idx]->cconf_out(global_socket_handlers[idx],0,0) == -1)
      return -1;
    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d created %s, socket %d",
             __FILE__, __LINE__,
             global_socket_handlers[idx]->name,
             global_socket_handlers[idx]->socket);
  }
#endif


  if (sw_shape_create() == -1)
    return -1;
//  for (int i = 0;i<MAX_PROC_RCV_CNT/4;++i)
//  {

  int outward_sp_id = process_qnty/2;
  if (sw_shape_queue_create(0) == -1 )
    return -1;
  if (sw_shape_queue_create(outward_sp_id) == -1 )
    return -1;
//  }
  if (sw_memgmt_create() == -1)
    return -1;

  if (sw_acct_scope_create() == -1) {
    return -1;
  }

  if (sw_acct_method_create() == -1) {
    return -1;
  }

  if (sw_acct_method_detail_create() == -1) {
    return -1;
  }
  
  if (sw_debug_flags & SW_DEBUG_CORE)
    sw_log("%s:%d acct backends created", __FILE__, __LINE__);



  shape_tick.tv_sec = 0;
  shape_tick.tv_usec = 0;
  shape_int.tv_sec = 0;
  shape_int.tv_usec = SW_SHAPE_QUEUE_PROCESS_FRQ;
  arg_th1 = 0;
  arg_th2 = process_qnty/2;
  int arg_i[process_qnty];

 // mutexesTableInit();

//  for (int i = 0;i<MAX_PROC_RCV_CNT;++i) {
//      arg_i[i] = i;
      pthread_create(&run_thread1[0], NULL, run, (void *)&arg_th1);
      pthread_create(&run_thread1[outward_sp_id], NULL, run, (void *)&arg_th2);
//  }




  rcv_in rcv_in_arr[process_qnty];
  for (int i = 0;i<process_qnty;i++) {
      rcv_in_arr[i].id = i;
      rcv_in_arr[i].swh = global_socket_handlers[i];
  }
  unrv.swh = global_socket_handlers[process_qnty];
  unrv.id = process_qnty;
  maintenance.filter_dir = unrv.filter_dir;
  maintenance.id = process_qnty + uam_thread_qnty;
  /*for (int i = 0;i<process_qnty;i++) {

      pthread_create(&rcv_thread[i], NULL, recv_thr, (void *)&rcv_in_arr[i]);
  }*/
  cpu_set_t cpuset;
         // Iterate over each thread
    for (int i = 0; i < process_qnty; i++) {
        CPU_ZERO(&cpuset); // Initialize CPU set to clear all CPUs
        CPU_SET(i % cpu_count, &cpuset); // Set the CPU affinity for the thread to the i-th core

        pthread_create(&rcv_thread[i], NULL, recv_thr, (void *)&rcv_in_arr[i]);

      watchdog_register(rcv_thread[i], i);
        // Set thread affinity for the created thread
        if (pthread_setaffinity_np(rcv_thread[i], sizeof(cpu_set_t), &cpuset) != 0) {
            //perror("pthread_setaffinity_np");
            // Handle error if needed
            sw_log("%s:%d cpu afinity error: %s", __FILE__, __LINE__, strerror(errno));
            
        }
    }

  for (int i = 0; i < uam_thread_qnty; i++) {
      uam_rcv_arr[i] = unrv;
      uam_rcv_arr[i].id = process_qnty + i;
      pthread_create(&uam_rcv_threads[i], NULL, uam_recv_thr, (void *)&uam_rcv_arr[i]);
      watchdog_register(uam_rcv_threads[i], process_qnty + i);
      CPU_ZERO(&cpuset);
      CPU_SET((process_qnty + i) % cpu_count, &cpuset);
      if (pthread_setaffinity_np(uam_rcv_threads[i], sizeof(cpu_set_t), &cpuset) != 0) {
          sw_log("%s:%d cpu afinity error: %s", __FILE__, __LINE__, strerror(errno));
      }
  }

  pthread_create(&maintenance_thread, NULL, maintenance_thr, (void *)&maintenance);
  watchdog_register(maintenance_thread, maintenance.id);
  CPU_ZERO(&cpuset);
  CPU_SET(maintenance.id % cpu_count, &cpuset);
  if (pthread_setaffinity_np(maintenance_thread, sizeof(cpu_set_t), &cpuset) != 0) {
      sw_log("%s:%d cpu afinity error: %s", __FILE__, __LINE__, strerror(errno));
  }


  pid_t pid = syscall(SYS_gettid);

  //int n = (unsigned int)pthread_self();

  //registerPerr();
time_t last_check = time(NULL);
  while(global_alive_flag)
  {

      struct timespec tw = {0,100};
      //Структура, в которую будет помещен остаток времени
      //задержки, если задержка будет прервана досрочно
      struct timespec tr;
      //Вывод сообщения о приостановки работы



      sleep(1);
      time_t now = time(NULL);
        if (now - last_check >= WATCHDOG_CHECK_INTERVAL_SEC && atomic_load(&g_watchdog_enabled)) {
            last_check = now;
            int max_idx = maintenance.id;
            if (max_idx >= WATCHDOG_MAX_THREADS) max_idx = WATCHDOG_MAX_THREADS - 1;
            for (int i = 0; i <= max_idx; ++i) {
                if (!g_watchdog[i].active) continue;
                long diff = now - atomic_load(&g_watchdog[i].last_ping);

                if (diff > WATCHDOG_TIMEOUT_SEC) {
                    sw_log("%s:%d [watchdog] worker %d stuck for %ld s - restarting",
       __FILE__, __LINE__, i, diff);

			//watchdog_run_script_and_kill(watchdog_script);
			watchdog_run_script_and_kill();
                }
            }
        }




   // pthread_rwlock_unlock(&inner_lock);

  }
     /*for (int i = 0;i<MAX_PROC_RCV_CNT;++i) {
         //pthread_join(threads[i], NULL);
         pthread_join(run_thread1[i], NULL);

     }*/
     for (int i = 0;i<process_qnty;++i) {

         pthread_cancel(rcv_thread[i]);

     }
     

     for (int i = 0; i < uam_thread_qnty; ++i) {
         pthread_cancel(uam_rcv_threads[i]);
     }
     pthread_cancel(maintenance_thread);
     pthread_cancel(run_thread1[0]);
     pthread_cancel(run_thread1[outward_sp_id]);
     sw_nat_src_stop_garbage();



  sw_session_destroy();
  sw_gauge_destroy();
  sw_shape_destroy();
  sw_shape_queue_destroy();

  sw_acct_commit(1);
  sw_acct_method_destroy();

  for(idx=0;global_socket_handlers[idx];idx++) {
    if (!global_socket_handlers[idx]->destroy)
      continue;
    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d destroying %s", __FILE__, __LINE__,
             global_socket_handlers[idx]->name);
    global_socket_handlers[idx]->destroy(global_socket_handlers[idx]);
  }

  free(r_in);
  free(r_out);
  free(global_socket_handlers);
  free(uam_rcv_arr);
  free(uam_rcv_threads);

  sw_lhstat_destroy();
  
  if (sw_debug_flags & (SW_DEBUG_CORE | SW_DEBUG_BREATH))
    sw_log("%s:%d sweetspot finished", __FILE__, __LINE__);

  return 0;
}
