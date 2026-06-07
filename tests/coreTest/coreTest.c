#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "src/nat_src.h"  // Include the lhash.h header file for the LHASH functions
#include "src/nat_src_endpoint_ip.h"
#include "src/nat_src_endpoint_tcp.h"
#include "src/nat_src_endpoint_icmp.h"
#include "src/tuple.h"
#include "src/tuple_ip.h"
#include "src/tuple_tcp.h"
#include "src/tuple_icmp.h"
#include "src/debug.h"
#include "src/cfg.h"
#include "src/session.h"
#include "src/pretty.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <pthread.h>
#include "src/sweetspot.h"
#include "src/netset.h"
#include "src/relay_inward.h"
#include "src/relay_outward.h"
#include "src/session.h"
#include "src/gauge.h"
#include "src/uamsrv.h"
#include "src/nat_dst.h"
#include "src/shape.h"
#include "src/memgmt.h"
#include "src/time_op.h"
#include "src/frag.h"
#include "src/cfg.h"
#include "src/acct.h"
#include "src/acct_scope.h"
#include "src/acct_method.h"
#include "src/acct_method_detail.h"
#include "src/filter.h"
#include "src/lhstat.h"
#include "src/log.h"
#include "src/sh_queue.h"
#include "src/relay_inward.h"
#include "src/relay_outward.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "src/filter_match.h"
#include "shape.h"
#include "src/nat_dst.h"
#include "src/nat_src.h"
#include "src/nat_dst_endpoint.h"
#include "src/nat_src_endpoint.h"
#include "src/frag.h"
#include "src/frag_tuple.h"
#include "src/nat_src_endpoint_ip.h"
#include <math.h>
#include "sweetspot.h"
//#include "src/mutexCirularBuffer.h"
//#include "src/cache.h"
#include <utime.h>
#include <errno.h>
#define NUM_THREADS 20
#define NUM_OPERATIONS 9000000
#define DIR_IN 1
#define DIR_OUT 2

#define LOW_IP 1684275713
#define HIGH_IP 1684275728
#define LOW_PORT 1000
#define HIGH_PORT 1050
#define DST_PORT_LOW 80
#define DST_PORT_HIGH 80


#define ACTION_START 0
#define ACTION_STOP 1
#define ACTION_STATUS 2
#define ACTION_VER_STATE 3
#define  ACTION_ACQ_ST_ID 4
#define ACTION_RETEN 5
#define ACTION_GAUGE_CUR 6
#define ACTION_GAUGE_PEAK 7
#define ACTION_GAUGE_RETEN 8
#define SESSION_STRESS_RUNTIME_SEC 5

int global_alive_flag = 1;
int process_qnty = 0;
sw_socket_handler_t ** global_socket_handlers;





#define BUFFER_SIZE 1000


typedef struct
{
    sw_tuple_t* pub;
    sw_tuple_t* prv;
} nat_translatiion_tuple_t;


typedef struct {
    nat_translatiion_tuple_t* data[BUFFER_SIZE];
    int read_index;
    int write_index;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

CircularBuffer buffer;
#include <regex.h>

static void randomize_redir_ip(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        sw_log("%s:%d [WARN] cannot open %s: %s", __FILE__, __LINE__, filepath, strerror(errno));
        return;
    }

    char *buf = NULL;
    size_t size = 0;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = malloc(size + 1);
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);

    // Найдём "redir to <ip>" и заменим IP
    char *ptr = buf;
    char outbuf[65536];
    size_t outlen = 0;

    while (*ptr) {
        char *redir = strstr(ptr, "redir to ");
        if (!redir) {
            strcpy(outbuf + outlen, ptr);
            break;
        }

        // Скопировать часть до "redir to "
        size_t prefix_len = redir - ptr;
        memcpy(outbuf + outlen, ptr, prefix_len);
        outlen += prefix_len;

        // Добавляем сам "redir to "
        memcpy(outbuf + outlen, "redir to ", 9);
        outlen += 9;
        ptr = redir + 9;

        // Генерируем случайный IP
        int a = 194;  // первая часть фиксирована
        int b = 67;   // вторая часть фиксирована
        int c = rand() % 255;
        int d = rand() % 255;

        char ipbuf[32];
        snprintf(ipbuf, sizeof(ipbuf), "%d.%d.%d.%d ", a, b, c, d);
        int n = snprintf(outbuf + outlen, sizeof(outbuf) - outlen, "%s", ipbuf);
        outlen += n;

        // Найдём остаток до конца строки
        while (*ptr && *ptr != '\n') {
            if (strncmp(ptr, "port", 4) == 0) break;
            ptr++;
        }

        // Дописать остаток строки (обычно " port ...")
        while (*ptr && *ptr != '\n') {
            outbuf[outlen++] = *ptr++;
        }

        if (*ptr == '\n') {
            outbuf[outlen++] = *ptr++;
        }

//        sw_log("%s:%d [DEBUG] rewrote redir to %s in %s", __FILE__, __LINE__, ipbuf, filepath);
    }

    outbuf[outlen] = '\0';

    // Перезаписываем файл
    fp = fopen(filepath, "w");
    if (!fp) {
        sw_log("%s:%d [WARN] cannot rewrite %s: %s", __FILE__, __LINE__, filepath, strerror(errno));
        free(buf);
        return;
    }
    fwrite(outbuf, 1, outlen, fp);
    fclose(fp);

    free(buf);
}

void init_buffer_ts(CircularBuffer *buffer) {
    buffer->read_index = 0;
    buffer->write_index = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
    pthread_cond_init(&buffer->not_full, NULL);
}

void enqueue_ts(CircularBuffer *buffer, nat_translatiion_tuple_t* value) {
    pthread_mutex_lock(&buffer->mutex);

    while ((buffer->write_index + 1) % BUFFER_SIZE == buffer->read_index) {
        // Buffer is full, wait for space
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    buffer->data[buffer->write_index] = value;
    buffer->write_index = (buffer->write_index + 1) % BUFFER_SIZE;

    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
}

nat_translatiion_tuple_t* dequeue_ts(CircularBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);

    while (buffer->read_index == buffer->write_index) {
        // Buffer is empty, wait for data
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    nat_translatiion_tuple_t* value = buffer->data[buffer->read_index];
    buffer->read_index = (buffer->read_index + 1) % BUFFER_SIZE;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);

    return value;
}


uint32_t generate_random_uint32(uint32_t low_bound, uint32_t high_bound) {
    if (low_bound > high_bound) {
        // Ensure low_bound is less than or equal to high_bound
        uint32_t temp = low_bound;
        low_bound = high_bound;
        high_bound = temp;
    }

    // Calculate the range of values
    uint32_t range = high_bound - low_bound + 1;

    // Generate a random number within the range
    return (rand() % range) + low_bound;
}

sw_tuple_t * tuple_generator(int ip, int port, int stop)
{
    sw_tuple_t * tuple = (sw_tuple_t*)malloc(sizeof(sw_tuple_t));
    memset(tuple,0x0,sizeof(sw_tuple_t));
    sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)malloc(sizeof(sw_tuple_ip_t));
    memset(sw_tuple_ip,0x0,sizeof(sw_tuple_ip_t));
    tuple->protocol = IPPROTO_IP;
    sw_tuple_ip->ip = htonl(ip);
    int res = generate_random_uint32(0,0);
    uint16_t flags_and_offset = 0;

    // Set the More Fragments (MF) flag to 1
    //flags_and_offset |= 0x2000;


    if(res)
    {
        // Set the Fragment Offset to the appropriate value (e.g., 3 for an offset of 24 bytes)
        flags_and_offset |= 3;
    sw_tuple_ip->frag_off = htons(flags_and_offset);
    }
    //else
    //sw_tuple_ip->frag_off = htons(flags_and_offset);;
    tuple->prv =  (sw_tuple_ip_t*)sw_tuple_ip;
//    sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)malloc(sizeof(sw_tuple_tcp_t));
//    memset(sw_tuple_tcp,0x0,sizeof(sw_tuple_tcp_t));
//    tuple->next = (sw_tuple_t*)malloc(sizeof(sw_tuple_t));
//    memset(tuple->next,0x0,sizeof(sw_tuple_t));
//    sw_tuple_tcp->port = htons(port);
//    tuple->next->prv = (sw_tuple_tcp_t*) sw_tuple_tcp;
//    tuple->next->protocol = IPPROTO_TCP;

    sw_tuple_icmp_t * sw_tuple_icmp = (sw_tuple_icmp_t*)malloc(sizeof(sw_tuple_icmp_t));
    memset(sw_tuple_icmp,0x0,sizeof(sw_tuple_icmp_t));
    tuple->next = (sw_tuple_t*)malloc(sizeof(sw_tuple_t));
    memset(tuple->next,0x0,sizeof(sw_tuple_t));
    sw_tuple_icmp->seq = htons(port);
    tuple->next->prv = (sw_tuple_icmp_t*) sw_tuple_icmp;
    tuple->next->protocol = IPPROTO_ICMP;
    if (stop == 0)
        tuple->next->next = tuple_generator(ip, port, 1);
    else
        tuple->next->next = NULL;
    return tuple;
}


int tuple_checker_src(sw_tuple_t * tuple, int direction)
{
    float global_k = ceilf((float)sw_netset_size(global_pub_ns)/sw_netset_size(global_ns));
    uint32_t global_port_chunk = (int)((65536-1024)/global_k);
    if(tuple->next == NULL) return 0;
    if(direction == DIR_IN)
    {
        sw_tuple_t * tuple_ = tuple;

        if(tuple->protocol != IPPROTO_IP && tuple->next->protocol != IPPROTO_TCP  && tuple->next->protocol != IPPROTO_ICMP)
        {

            sw_log("%s:%d wrong protocol:%s tuple:%s", __FILE__, __LINE__,sw_pretty_proto(tuple->protocol), sw_tuple_pretty(tuple_));
            return -1;
        }
        sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)tuple->prv;
        if(!sw_check_ip(global_pub_ns,ntohl(sw_tuple_ip->ip)))
        {

            sw_log("%s:%d wrong IP tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(tuple_));
            return -1;
        }
        uint32_t prv_num = sw_netset_idx(global_ns, ntohl(sw_tuple_ip->ip));
        //printf("\n prv_num %d",prv_num);
        //printf("\n global_k %f",global_k);
        //printf("\n global_port_chunk %d",global_port_chunk);
        uint16_t lport = (int)((prv_num % (int)global_k) * global_port_chunk + 1024);
        uint16_t hport = lport + global_port_chunk - 1;
        /*sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)tuple->next->prv;
        if(ntohs(sw_tuple_tcp->port) < lport || ntohs(sw_tuple_tcp->port) > hport)
        {
            sw_log("%s:%d wrong PORT tuple:%s should be in range: low - %d, high - %d", __FILE__, __LINE__, sw_tuple_pretty(tuple_),lport,hport);
            return -1;
        }*/
    }
    else
    {
        sw_tuple_t * tuple_ = tuple;
        if(tuple->protocol != IPPROTO_IP || tuple->next->protocol != IPPROTO_TCP || tuple->next->protocol != IPPROTO_ICMP)
        {
             sw_log("%s:%d wrong protocol:%s tuple:%s", __FILE__, __LINE__,sw_pretty_proto(tuple->protocol), sw_tuple_pretty(tuple_));
            return -1;
        }
        sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)tuple->prv;
        if(ntohl(sw_tuple_ip->ip) < LOW_IP || ntohl(sw_tuple_ip->ip) > HIGH_IP)
        {
            sw_log("%s:%d wrong IP tuple:%s should be in range: low-%s high-%s"
                   " ", __FILE__, __LINE__, sw_tuple_pretty(tuple_), sw_pretty_ip(LOW_IP),sw_pretty_ip(HIGH_IP));
            return -1;
        }
        sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)tuple->next->prv;
        /*if(ntohs(sw_tuple_tcp->port) < DST_PORT_LOW || ntohs(sw_tuple_tcp->port) > DST_PORT_HIGH)
        {
            sw_log("%s:%d wrong PORT tuple:%s should be in range: low - %d, high - %d", __FILE__, __LINE__, sw_tuple_pretty(tuple_),DST_PORT_LOW,DST_PORT_HIGH);
            return -1;
        }*/
    }
    return 0;

};


int tuple_checker_dst(sw_tuple_t * tuple, int direction)
{
    uint32_t global_k = ceilf((float)sw_netset_size(global_ns)/sw_netset_size(global_pub_ns));
    float global_port_chunk = (int)((65536-1024)/global_k);
     if(tuple->next == NULL) return 0;
    if(direction == DIR_IN)
    {
        sw_tuple_t * tuple_ = tuple;
        if(tuple->protocol != IPPROTO_IP && (tuple->next->protocol != IPPROTO_TCP || tuple->next->protocol != IPPROTO_ICMP))
        {
             sw_log("%s:%d wrong protocol:%s tuple:%s", __FILE__, __LINE__,sw_pretty_proto(tuple->protocol), sw_tuple_pretty(tuple_));
            return -1;
        }
        sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)tuple->prv;
        if(ntohl(sw_tuple_ip->ip) < LOW_IP || ntohl(sw_tuple_ip->ip) > HIGH_IP)
        {
            sw_log("%s:%d wrong IP tuple:%s should be in range: low-%s high-%s"
                   " ", __FILE__, __LINE__, sw_tuple_pretty(tuple_), sw_pretty_ip(LOW_IP),sw_pretty_ip(HIGH_IP));
            return -1;
        }
        sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)tuple->next->prv;
//        if(ntohs(sw_tuple_tcp->port) < DST_PORT_LOW || ntohs(sw_tuple_tcp->port) > DST_PORT_HIGH)
//        {
//            sw_log("%s:%d wrong PORT tuple:%s should be in range: low - %d, high - %d", __FILE__, __LINE__, sw_tuple_pretty(tuple_),DST_PORT_LOW,DST_PORT_HIGH);
//            return -1;
//        }
    }
    else
    {
        sw_tuple_t * tuple_ = tuple;
        if(tuple->protocol != IPPROTO_IP && (tuple->next->protocol != IPPROTO_TCP || tuple->next->protocol != IPPROTO_ICMP))
        {
             sw_log("%s:%d wrong protocol:%s tuple:%s", __FILE__, __LINE__,sw_pretty_proto(tuple->protocol), sw_tuple_pretty(tuple_));
            return -1;
        }
        sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)tuple->prv;
        if(!sw_check_ip(global_ns,ntohl(sw_tuple_ip->ip)))
        {
            sw_log("%s:%d wrong IP tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(tuple_));
            return -1;
        }
        sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)tuple->next->prv;
//        if(ntohs(sw_tuple_tcp->port) < LOW_PORT || ntohs(sw_tuple_tcp->port) > HIGH_PORT)
//        {

//            sw_log("%s:%d wrong PORT tuple:%s should be in range: low - %d, high - %d", __FILE__, __LINE__, sw_tuple_pretty(tuple_),LOW_PORT,HIGH_PORT);

//            return -1;
//        }
    }
    return 0;

};




int proc_inner(CircularBufferGauge * cbg, StaticCacheNode * scn, sw_tuple_t ** src_in, sw_tuple_t ** dst_in) {
  sw_tuple_t *osrc, *src, *dst, *odst, *nat;
  sw_filter_rule_t *rule = NULL;
  sw_memgmt_buf_t *p_buf;
  sw_tuple_t *hsrc, *hdst;
  hsrc = hdst = NULL;
  int delaytime, shape_flag;
  CachVal * cv;

  src = *src_in;
  dst = *dst_in;

  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d processing inward packet", __FILE__, __LINE__);



  if (sw_debug_flags & SW_DEBUG_RELAY)
    sw_log("%s:%d from %s to %s", __FILE__, __LINE__,
           sw_tuple_pretty(src), sw_tuple_pretty(dst));

  //pthread_rwlock_wrlock(mtx);
  if (sw_frag_do(&hsrc, &hdst, src, dst) == -1) {
    sw_tuple_free(src); sw_tuple_free(dst);
    sw_tuple_free(hsrc); sw_tuple_free(hdst);
    return 0; /* Drop */
  }


  nat = osrc = odst = NULL;

  pthread_rwlock_rdlock(&global_filter_lock);
  int fm_res = 0;
  uint32_t cache_key = sw_tuple_hash_simpl(hsrc ? hsrc : src) ^ sw_tuple_hash_simpl(hdst ? hdst : dst);
  //sw_log("%s:%d filter for relation found in cache %d , src:%d, dst:%d",
    //     __FILE__, __LINE__, cache_key, sw_tuple_hash_simpl(hsrc ? hsrc : src),sw_tuple_hash_simpl(hdst ? hdst : dst));
  if(get(scn,cache_key,&cv))
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
        put(scn,cache_key,tcv);
    }
    if (sw_debug_flags & SW_DEBUG_RELAY)
      sw_log("%s:%d filter for relation not found in cache %s->%s",
             __FILE__, __LINE__, sw_tuple_pretty(src),
             sw_tuple_pretty(dst));
  }

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
      pthread_rwlock_unlock(&global_filter_lock);
      return 0;
    case SW_FILTER_ACTION_DNAT:
    case SW_FILTER_ACTION_DNAT|SW_FILTER_ACTION_SHAPE:
      if (hdst ? sw_frag_tuple_clone(&nat, hdst): \
          sw_tuple_clone(&nat, dst) == -1) {
        if(src)sw_tuple_free(src); if(dst)sw_tuple_free(dst);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
        pthread_rwlock_unlock(&global_filter_lock);
        return -1;
      }
      if (sw_nat_dst_endpoint_assign(nat, htonl(rule->dnat.ip),
                                     htons(rule->dnat.port)) == -1) {
        sw_tuple_free(src); sw_tuple_free(dst); sw_tuple_free(nat);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
        pthread_rwlock_unlock(&global_filter_lock);
        return -1;
      }
     //pthread_rwlock_wrlock(mtx);
      if (sw_nat_dst_do(hsrc ? hsrc : src, hdst ? hdst : dst, nat) == -1) {
        sw_tuple_free(src); sw_tuple_free(dst); sw_tuple_free(nat);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
        pthread_rwlock_unlock(&global_filter_lock);
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
                  if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
      pthread_rwlock_unlock(&global_filter_lock);
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
    pthread_rwlock_unlock(&global_filter_lock);
    return 0; /* Drop */
  }
  pthread_rwlock_unlock(&global_filter_lock);

  //releaseAllMtx();

  //pthread_rwlock_wrlock(mtx);

  if (sw_gauge_drop_stat(cbg,hsrc ? hsrc : src, 1500, SW_GAUGE_DIR_IN) == -1) {
    sw_tuple_free(src); sw_tuple_free(dst);
          if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
    if (odst) sw_tuple_free(odst);

    return -1;
  }
  //pthread_rwlock_unlock(mtx);

  if (sw_nat_src_enabled_flag) {
    if (sw_nat_src_do(&nat, hsrc ? hsrc : src) == -1) {
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




  if (osrc) sw_tuple_free(osrc); if (odst) sw_tuple_free(odst);
  if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
  if(rule->action != (SW_FILTER_ACTION_DNAT|SW_FILTER_ACTION_SHAPE) && rule->action !=SW_FILTER_ACTION_DNAT)
  if((tuple_checker_src(src,DIR_IN) == -1 || tuple_checker_dst(dst,DIR_IN) == -1))
  {
      sw_log("\n drop");
      sw_tuple_free(src); sw_tuple_free(dst);
      return -1;
  }
  //sw_tuple_free(src); sw_tuple_free(dst);
  //return -1;
  *src_in = src;
  *dst_in = dst;
  return 1;
}

int proc_outer(CircularBufferGauge * cbg, StaticCacheNode * scn, sw_tuple_t ** src_in, sw_tuple_t ** dst_in) {
    sw_tuple_t  *osrc, *src, *dst, *odst, *nat;
    sw_filter_rule_t *rule = NULL;
    sw_memgmt_buf_t *p_buf;
    int delaytime, shape_flag;
    sw_tuple_t *hsrc, *hdst;
    hsrc = hdst = NULL;
    dst = *dst_in;
    src = *src_in;
    CachVal * cv;


    if (sw_debug_flags & SW_DEBUG_RELAY)
      sw_log("%s:%d processing outward packet", __FILE__, __LINE__);

  //  if ((length = sw_if_recv(self->socket, packet, sizeof(packet))) == -1) {
  //    return -1;
  //  }






    //pthread_rwlock_wrlock(mtx);
    if (sw_frag_do(&hsrc, &hdst, src, dst) == -1) {
      sw_tuple_free(src); sw_tuple_free(dst);
      sw_tuple_free(hsrc); sw_tuple_free(hdst);


      //pthread_rwlock_unlock(mtx);
      return 0; /* Drop */
    }


    nat = osrc = odst = NULL;

    //pthread_rwlock_wrlock(mtx);
    if (sw_nat_src_enabled_flag) {
      if (sw_nat_src_undo(&nat, hdst ? hdst : dst) == -1) {
        sw_tuple_free(src); sw_tuple_free(dst);
        if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
        if(nat)sw_tuple_free(nat);

        return 0; /* Drop */
      }


      odst = dst;
      hdst ? sw_frag_tuple_clone(&dst, nat) : sw_tuple_clone(&dst, nat);
      if (hdst)
        hdst = nat;
      else
      {
        if(nat)sw_tuple_free(nat);

      }
      nat = NULL;
    }


    //pthread_rwlock_wrlock(mtx);
    pthread_rwlock_rdlock(&global_filter_lock);
    uint32_t cache_key = sw_tuple_hash_simpl(hsrc ? hsrc : src) ^ sw_tuple_hash_simpl(hdst ? hdst : dst);
    //sw_log("%s:%d filter for relation found in cache %d , src:%d, dst:%d",
    //       __FILE__, __LINE__, cache_key, sw_tuple_hash(hsrc ? hsrc : src),sw_tuple_hash(hdst ? hdst : dst));
    int fm_res= 0;
    if(get(scn,cache_key,&cv))
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
                                 SW_FILTER_MATCH_OUTWARD);
      if(fm_res>=0)
      {
          CachVal tcv;
          tcv.rule = rule;
          tcv.type = fm_res;
          put(scn,cache_key,tcv);
      }
      if (sw_debug_flags & SW_DEBUG_RELAY)
        sw_log("%s:%d filter for relation not found in cache %s->%s",
               __FILE__, __LINE__, sw_tuple_pretty(src),
               sw_tuple_pretty(dst));
    }

    //sw_log("%s:%d outward mtx bloking 1 pos: %d", __FILE__, __LINE__, getBlockQty());
    //
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
        sw_tuple_free(src); sw_tuple_free(dst);
        if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
        if (odst) sw_tuple_free(odst);
        pthread_rwlock_unlock(&global_filter_lock);
        return 0;
      case SW_FILTER_ACTION_DNAT:
      case SW_FILTER_ACTION_DNAT|SW_FILTER_ACTION_SHAPE:
          //pthread_rwlock_wrlock(mtx);
        if (sw_nat_dst_undo(&nat, hsrc ? hsrc : src, hdst ? hdst : dst) == -1) {
          sw_tuple_free(src); sw_tuple_free(dst);
          if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
      if (odst) sw_tuple_free(odst);
      if(nat) sw_tuple_free(nat);

          pthread_rwlock_unlock(&global_filter_lock);
          return 0; /* Drop */
        }
        //pthread_rwlock_unlock(mtx);
        if (sw_debug_flags & SW_DEBUG_RELAY)
          sw_log("%s:%d packet %s->%s undone DNAT from %s%s", __FILE__, __LINE__,
                 sw_tuple_pretty(src), sw_tuple_pretty(dst),
                 sw_tuple_pretty(nat),
                 shape_flag ? " & shaped" : "");
        osrc = src;
        sw_tuple_clone(&src, nat);
        if(nat) sw_tuple_free(nat);
        break; /* Re-dir and pass */
      case SW_FILTER_ACTION_SHAPE:
        if (sw_debug_flags & (SW_DEBUG_RELAY|SW_DEBUG_SHAPE))
          sw_log("%s:%d matched packet %s->%s shaped filter",
                 __FILE__, __LINE__, sw_tuple_pretty(src),
                 sw_tuple_pretty(dst));
        break;
      default:
        sw_log("%s:%d unexpected action %d", __FILE__, __LINE__, rule->action);
        if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
        sw_tuple_free(src); sw_tuple_free(dst);
        if (odst) sw_tuple_free(odst);
        pthread_rwlock_unlock(&global_filter_lock);

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
      if (odst) sw_tuple_free(odst);
      //sw_log("%s:%d outward mtx bloking 2 pos: %d", __FILE__, __LINE__, getBlockQty());

      pthread_rwlock_unlock(&global_filter_lock);
      return 0; /* Drop */
    }

    pthread_rwlock_unlock(&global_filter_lock);
    if (sw_gauge_drop_stat(cbg,hdst ? hdst : dst, 1500, SW_GAUGE_DIR_OUT) == -1) {
        if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
      sw_tuple_free(src); sw_tuple_free(dst);
      if (osrc) sw_tuple_free(osrc); if (odst) sw_tuple_free(odst);



      return -1;
    }
   /* if(rule->action != (SW_FILTER_ACTION_DNAT|SW_FILTER_ACTION_SHAPE) && rule->action !=SW_FILTER_ACTION_DNAT)
    if((tuple_checker_src(src,DIR_OUT) == -1 || tuple_checker_dst(dst,DIR_OUT) == -1))
    {
        if (osrc) sw_tuple_free(osrc); if (odst) sw_tuple_free(odst);
        sw_tuple_free(src); sw_tuple_free(dst);
              if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
              return -1;
    }*/
      /* if sw_if_send failed => packet dropped */
    if (osrc) sw_tuple_free(osrc); if (odst) sw_tuple_free(odst);
    sw_tuple_free(src); sw_tuple_free(dst);
          if(hsrc)sw_tuple_free(hsrc); if(hdst)sw_tuple_free(hdst);
          //if(nat)sw_tuple_free(nat);




    return 0;


}



static int usage() {
  fprintf(stderr, "Usage: sweetspot [ -hVf ] [ -d debug-what ] [ -c config-file ] [ -l log-file ]\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -h              this help message\n");
  fprintf(stderr, "  -V              software version information\n");
  fprintf(stderr, "  -f              stay foreground\n");
  fprintf(stderr, "  -d debug-what   [%s]\n", sw_debug_keywords());
  fprintf(stderr, "  -c filename     configuration file (default %s)\n",
          "coreTest/test.conf");
  fprintf(stderr, "  -l filename     log file (default %s)\n",
          SW_SWEETSPOT_LOG_FILE);
  return 0;
}


void* thread_proc_run(void* arg)
{

        float global_k = ceilf((float)sw_netset_size(global_pub_ns)/sw_netset_size(global_ns));
        uint32_t global_port_chunk = (int)((65536-1024)/global_k);
        int num = (int)arg;
        num /= 2;
        nat_translatiion_tuple_t * nat_tr;
        uint32_t ip_min, ip_max;
        ip_min = 0;
        ip_max = sw_netset_size(global_ns)-1;
        StaticCache * cache = malloc(sizeof(StaticCache));
        CircularBufferGauge * cBuf = malloc(sizeof(CircularBufferGauge));
        initCache(cache);
        init_buffer(cBuf);
        gauge_buffer_register(cBuf);
        
    //    global_socket_handlers[num]->sp = malloc(sizeof(spinlock));
        /*int window_size = ((ip_max - ip_min)*2)/NUM_THREADS;
        ip_min += num*window_size;
        ip_max = ip_min + window_size-1;*/
        sw_log("\n ip_min: %s   ip_max: %s ", sw_pretty_ip(sw_netset_ip(global_ns,ip_min)),sw_pretty_ip(sw_netset_ip(global_ns,ip_max)));
        //return 0;
        int i;
        for (i = 0; i < NUM_OPERATIONS; i++) {
            nat_tr =  malloc(sizeof(nat_translatiion_tuple_t));
            uint32_t random_number = sw_netset_ip(global_ns,generate_random_uint32(ip_min, ip_max));
            uint32_t random_port = generate_random_uint32(LOW_PORT, HIGH_PORT);
            uint32_t random_dst_ip = generate_random_uint32(LOW_IP, HIGH_IP);
            uint32_t random_dst_port = generate_random_uint32(DST_PORT_LOW, DST_PORT_HIGH);
            sw_tuple_t * src = tuple_generator(random_number, random_port, 0);
            sw_tuple_t * dst = tuple_generator(random_dst_ip, random_dst_port, 0);
            //sw_log("%s:%d push tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(src));
        spinlock_lock(global_socket_handlers[num]->sp);
            if(proc_inner(cBuf,cache,&src,&dst)== 1)
            {

                //sw_log("%s:%d push tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(src));
                    nat_tr->prv = src;
                    nat_tr->pub = dst;
            //sw_log("%s:%d push tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(nat_tr->prv));
            //sw_log("%s:%d push tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(nat_tr->pub));
            enqueue_ts(&buffer,nat_tr);
            }
            else
            {
                free(nat_tr);
            }

        spinlock_unlock(global_socket_handlers[num]->sp);
        }
        printf("total nums ops :%d", i);
        return NULL;
};

void* thread_proc_run_out(void* arg)
{
    nat_translatiion_tuple_t * nat_tr;
    uint32_t ip_min, ip_max;
    ip_min = global_ns->ip_min;
    int num = (int)arg;
    for(sw_netset_t * ns = global_ns;ns;ns = ns->next)
    {
        ip_max = ns->ip_max;
    }
    //printf("\n ip_min: %s   ip_max: %s ", sw_pretty_ip(ip_min),sw_pretty_ip(ip_max));

    time_t last_print = time(NULL);
    int i;
    StaticCache * cache = (StaticCache *)malloc(sizeof(StaticCache));
    CircularBufferGauge * cBuf = (CircularBufferGauge *)malloc(sizeof(CircularBufferGauge));
    initCache(cache);
    init_buffer(cBuf);
    gauge_buffer_register(cBuf);

   //   global_socket_handlers[num]->sp = malloc(sizeof(spinlock));
    int prev_num  = 0;
    for (i = 0; i < NUM_OPERATIONS; i++) {
        nat_tr  = dequeue_ts(&buffer);
        spinlock_lock(global_socket_handlers[num]->sp);
        proc_outer(cBuf,cache,&nat_tr->pub, &nat_tr->prv);
        free(nat_tr);

        spinlock_unlock(global_socket_handlers[num]->sp);
        time_t now = time(NULL);

        if (now - last_print >= 5) {
            printf("thread id:%d implemented ops:%d total_num ops:%d  diff:%d \n",num, i, NUM_OPERATIONS,(i-prev_num));
            last_print = now;
            prev_num = i;
        }

    }

    printf("total nums ops: %d\n", i);
    return NULL;
}

void touch_file(const char *filename) {
    struct utimbuf new_times;
    time_t now = time(NULL);
    new_times.actime = now;   // время последнего доступа
    new_times.modtime = now;  // время последней модификации
    if (utime(filename, &new_times) == 0) {
        //sw_log("%s:%d [DEBUG] touched filter file: %s (mtime updated to %ld)",
         //      __FILE__, __LINE__, filename, now);
    } else {
       // sw_log("%s:%d [WARN] failed to touch filter file: %s",
         //      __FILE__, __LINE__, filename);
    }
}

void* thread_proc_session(void* arg)
{


    float global_k = ceilf((float)sw_netset_size(global_pub_ns)/sw_netset_size(global_ns));
    uint32_t global_port_chunk = (int)((65536-1024)/global_k);
        uint32_t ip_min, ip_max;
        char * filter_names;
        uint64_t octets_in, octets_out, bps_in, bps_out;
         time_t duration, idle, interim_interval;

         ip_min = 0;
         ip_max = sw_netset_size(global_ns)-1;

         /*int window_size = ((ip_max - ip_min)*2)/NUM_THREADS;
         ip_min += num*window_size;
         ip_max = ip_min + window_size-1;*/
         sw_log("\n ip_min: %s   ip_max: %s ", sw_pretty_ip(sw_netset_ip(global_ns,ip_min)),sw_pretty_ip(sw_netset_ip(global_ns,ip_max)));

 const char *filter_dir = sw_config_get("filter-dir", SW_FILTER_DIR);
        int state,cause;
        uint32_t id;
        time_t inherim;
        char * context;
        int st_id;

        /*for(int i = ip_min ; i<= ip_max; i++)
        {
            while(1)
            {
                uint32_t random_filter = generate_random_uint32(0, SW_FILTER_SLOTS_MAX-1);
                int res = sw_filter_get_name((const char**)&filter_names,random_filter);
                if(res==0)
                break;
            }
            sw_session_start(i,filter_names,60,"","");

        }*/


        while (global_alive_flag) {
            while(1)
            {
                uint32_t random_filter = generate_random_uint32(0, SW_FILTER_SLOTS_MAX-1);
                int res = sw_filter_get_name((const char**)&filter_names,random_filter);
                if(res==0)
                break;
            }
            uint32_t random_ip = sw_netset_ip(global_ns,generate_random_uint32(ip_min, ip_max));
            uint32_t action_num = generate_random_uint32(0, 8);
            usleep(100);
            switch (action_num) {
            case ACTION_START:
                sw_gauge_new(random_ip,100000000,1000000000,10000000,100000000,inherim,inherim);
                sw_session_start(random_ip,filter_names,60,"","");
            break;
            case ACTION_STOP:
                sw_session_stop(random_ip,filter_names,60,"","");
            break;
            case ACTION_STATUS:
                sw_session_status(random_ip,&state,&id,(char**)&filter_names,&inherim,(char**)&context,&cause);
            break;
            case ACTION_VER_STATE:
                sw_session_verify_stateid(random_ip,&st_id);
                break;
            case ACTION_ACQ_ST_ID:
                sw_session_acquire_stateid(random_ip,&st_id,1);
            break;
            case ACTION_RETEN:
                sw_session_retention(random_ip,&inherim);
            break;
            case ACTION_GAUGE_CUR:
                sw_gauge_current(random_ip,
                                         &octets_in, &octets_out,
                                         NULL, NULL,
                                         &bps_in, &bps_out,
                                         &duration, &idle,1);
                break;
            case ACTION_GAUGE_PEAK:
                sw_gauge_peak(random_ip, &bps_in, &bps_out,1);
                break;
            case ACTION_GAUGE_RETEN:
                sw_gauge_limits(random_ip,&octets_in, &octets_out,
                                &bps_in, &bps_out,
                                &duration, &idle,1);
                break;
            default:
                 sw_log("%s:%d wrong action type", __FILE__, __LINE__);

            }
                    // обновляем время модификации файла фильтра
	        char path[512];
	        snprintf(path, sizeof(path), "%s/%s", filter_dir, filter_names);
	        touch_file(path);
	        randomize_redir_ip(path);
	        }
	        return NULL;
};

void* thread_proc_maintenance(void* arg)
{
    const char *filter_dir = sw_config_get("filter-dir", SW_FILTER_DIR);

    while (global_alive_flag) {
        time_t now = time(NULL);
        sw_lhstat_do(now);
        sw_gauge_expire();
        sw_session_expire();
        sw_acct_commit(0);
        sw_filter_reload(filter_dir, now);
        usleep(1000);
    }

    return NULL;
}



int main(int argc, char **argv) {
    pthread_t threads[NUM_THREADS+2];
    init_buffer_ts(&buffer);
    fd_set fds;
    struct timeval tout, curr_time, shape_tick, shape_int;

    int maxfd, idx, p_len;
    char *config_filename = "coreTest/test.conf";
    char *log_filename = "coreTest.log";
    char *debug_keywords = NULL;
    int c, foreground_flag = 0;
    int arg_th1, arg_th2;
    extern char *optarg;


    if (pthread_rwlock_init(&global_filter_lock, NULL) != 0) {
          // Error handling: Initialization failed
          perror("pthread_rwlock_init");
          return 0;
      }



    while ((c = getopt(argc, argv, "hVfd:c:l:")) != -1) {
      switch (c) {
      case 'h':
        usage();
        return 0;
      case 'V':
        fprintf(stderr, "sweetspot version %s\n", "0.1");
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
      sw_log("%s:%d sweetspot version %s starting", __FILE__, __LINE__, "1.0");

    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d config %s loaded", __FILE__, __LINE__, config_filename);




    if (sw_lhstat_create() == -1)
      return -1;

    if (sw_filter_create(sw_config_get("filter-dir", SW_FILTER_DIR)) == -1) {
      return -1;
    }

    if (sw_session_create() == -1) {
      return -1;
    }

    if (sw_gauge_create() < 0)
      return -1;

    if (sw_nat_dst_create() == -1)
      return -1;

    if (sw_frag_create() == -1)
      return -1;

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

    if (sw_shape_create() == -1)
      return -1;
  //  for (int i = 0;i<MAX_PROC_RCV_CNT/4;++i)
  //  {


    if (sw_shape_queue_create(0) == -1 )
      return -1;




    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d acct backends created", __FILE__, __LINE__);


  process_qnty = NUM_THREADS;
  if (process_qnty == -1 || process_qnty % 2 != 0) {
      sw_log("%s:%d thread_qnty parameter is incorrect or thread_qnty mod 2 > 0", __FILE__, __LINE__);
            return -1;
  }
sw_socket_handler_t * r_in;
sw_socket_handler_t * r_out;


  r_in = malloc(sizeof(sw_socket_handler_t) * process_qnty/2);
  r_out = malloc(sizeof(sw_socket_handler_t) * process_qnty/2);
  global_socket_handlers = malloc(sizeof(sw_socket_handler_t*) * (process_qnty + 2));

  sw_socket_handler_t relay = {
  "INWARD",
  0,
  0,
  0,
  0,
  0 ,
  0,
  0,
  0,
  0,
  0,
  0,
  NULL
};

  for(int i = 0; i<process_qnty/2;i++){
    r_in[i] = relay;
    r_out[i] = relay;
    r_in[i].sp = malloc(sizeof(spinlock));
    r_in[i].sp->locked = 0;
    r_out[i].sp = malloc(sizeof(spinlock));
    r_out[i].sp->locked = 0;
    global_socket_handlers[i] = &r_in[i];
    global_socket_handlers[i+process_qnty/2] = &r_out[i];
  }



    //int n = (unsigned int)pthread_self();

    //registerPerr();

    // Create threads for insertions, deletions, and retrievals
    for (int i = 0; i < NUM_THREADS; i++) {
         //pthread_create(&threads[i], NULL, thread_proc_run, NULL);
        if (i % 2 == 0) {
            pthread_create(&threads[i], NULL, thread_proc_run, (void*)i);
        } else {
            pthread_create(&threads[i], NULL, thread_proc_run_out, (void*)i);
        }
    }
    pthread_create(&threads[NUM_THREADS], NULL, thread_proc_session, NULL);
    pthread_create(&threads[NUM_THREADS + 1], NULL, thread_proc_maintenance, NULL);

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        if (i % 2 == 0) {
            pthread_join(threads[i],NULL);
        }
    }
    sleep(SESSION_STRESS_RUNTIME_SEC);
    global_alive_flag = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        if (i % 2 != 0)
            {
               pthread_cancel(threads[i]);
            }
    }
    pthread_join(threads[NUM_THREADS], NULL);
    pthread_join(threads[NUM_THREADS + 1], NULL);

    sw_session_destroy();
    sw_gauge_destroy();
    //sw_shape_destroy();
    //sw_shape_queue_destroy();

    sw_acct_commit(1);
    sw_acct_method_destroy();

    // Clean up and free the hash table


    return 0;
}
