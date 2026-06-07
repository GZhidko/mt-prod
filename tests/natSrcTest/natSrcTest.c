#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <unistd.h>
#include "src/nat_src.h"  // Include the lhash.h header file for the LHASH functions
#include "src/nat_src_endpoint_ip.h"
#include "src/nat_src_endpoint_tcp.h"
#include "src/tuple.h"
#include "src/tuple_ip.h"
#include "src/tuple_tcp.h"
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

#define NUM_THREADS 40
#define NUM_OPERATIONS 200000

#define DIR_IN 1
#define DIR_OUT 2

#define LOW_IP 1684275713
#define HIGH_IP 1684275723
#define LOW_PORT 1000
#define HIGH_PORT 1050
#define DST_PORT_LOW 80
#define DST_PORT_HIGH 80



#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 1000

int global_alive_flag = 1;
sw_socket_handler_t ** global_socket_handlers;
int process_qnty = 0;

typedef struct {
    sw_tuple_t* data[BUFFER_SIZE];
    int read_index;
    int write_index;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

CircularBuffer buffer;

void init_buffer_ts(CircularBuffer *buffer) {
    buffer->read_index = 0;
    buffer->write_index = 0;
    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
    pthread_cond_init(&buffer->not_full, NULL);
}

void enqueue_ts(CircularBuffer *buffer, sw_tuple_t* value) {
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

sw_tuple_t* dequeue_ts(CircularBuffer *buffer) {
    pthread_mutex_lock(&buffer->mutex);

    while (buffer->read_index == buffer->write_index) {
        // Buffer is empty, wait for data
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    sw_tuple_t* value = buffer->data[buffer->read_index];
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

sw_tuple_t * tuple_generator(int ip, int port)
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
    sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)malloc(sizeof(sw_tuple_tcp_t));
    memset(sw_tuple_tcp,0x0,sizeof(sw_tuple_tcp_t));
    tuple->next = (sw_tuple_t*)malloc(sizeof(sw_tuple_t));
    memset(tuple->next,0x0,sizeof(sw_tuple_t));
    sw_tuple_tcp->port = htons(port);
    tuple->next->prv = (sw_tuple_tcp_t*) sw_tuple_tcp;
    tuple->next->protocol = IPPROTO_TCP;
    tuple->next->next = NULL;
    return tuple;
}

int tuple_checker_nat(sw_tuple_t * tuple)
{
    float global_k = ceilf((float)sw_netset_size(global_pub_ns)/sw_netset_size(global_ns));
    uint32_t global_port_chunk = (int)((65536-1024)/global_k);
    if(tuple->next == NULL) return 0;

        sw_tuple_t * tuple_ = tuple;

        if(tuple->protocol != IPPROTO_IP || tuple->next->protocol != IPPROTO_TCP)
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
        sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)tuple->next->prv;
        if(ntohs(sw_tuple_tcp->port) < lport || ntohs(sw_tuple_tcp->port) > hport)
        {
            sw_log("%s:%d wrong PORT tuple:%s should be in range: low - %d, high - %d", __FILE__, __LINE__, sw_tuple_pretty(tuple_),lport,hport);
            return -1;
        }

    return 0;

};


int tuple_checker_dst(sw_tuple_t * tuple)
{
    uint32_t global_k = ceilf((float)sw_netset_size(global_ns)/sw_netset_size(global_pub_ns));
    float global_port_chunk = (int)((65536-1024)/global_k);
     if(tuple->next == NULL) return 0;

        sw_tuple_t * tuple_ = tuple;
        if(tuple->protocol != IPPROTO_IP || tuple->next->protocol != IPPROTO_TCP)
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
        if(ntohs(sw_tuple_tcp->port) < LOW_PORT || ntohs(sw_tuple_tcp->port) > HIGH_PORT)
        {

            sw_log("%s:%d wrong PORT tuple:%s should be in range: low - %d, high - %d", __FILE__, __LINE__, sw_tuple_pretty(tuple_),LOW_PORT,HIGH_PORT);

            return -1;
        }
    return 0;

};




void* do_operations(void* arg)
{

    uint32_t ip_min, ip_max;
    ip_min = global_ns->ip_min;
    for(sw_netset_t * ns = global_ns;ns;ns = ns->next)
    {
        ip_max = ns->ip_max;
    }
    printf("\n ip_min: %s   ip_max: %s ", sw_pretty_ip(ip_min),sw_pretty_ip(ip_max));
    sw_session_entry_t * tt;
    spinlock * sp = NULL;
    for(int i = ip_min ; i<= ip_max; i++)
    {
        sw_session_new(&tt,i,&sp);
        spinlock_unlock(sp);
    }

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        uint32_t random_number = generate_random_uint32(ip_min, ip_max);
        uint32_t random_port = generate_random_uint32(LOW_PORT, HIGH_PORT);
        uint32_t random_dst_ip = generate_random_uint32(LOW_IP, HIGH_IP);
        uint32_t random_dst_port = generate_random_uint32(DST_PORT_LOW, DST_PORT_HIGH);
        sw_tuple_t * prv = tuple_generator(random_number, random_port);
        sw_tuple_t * nat = NULL;
        sw_tuple_t * ret = NULL;
        sw_nat_src_do(&nat,prv);
        if(nat)
        {

            tuple_checker_nat(nat);
            //sw_log("%s:%d push tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(nat));
            enqueue_ts(&buffer, nat);
            //if(sw_tuple_cmp(ret,prv) != 0)
             //sw_log("%s:%d wrong undo nat operation tuple 1 :%s tuple 2 :%s", __FILE__, __LINE__, sw_tuple_pretty(prv),sw_tuple_pretty(ret));
            //if(ret) sw_tuple_free(ret);
            //sw_tuple_free(nat);
        }
        else
        {
            sw_log("%s:%d not nating for tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(prv));
        }
        sw_tuple_free(prv);
    }
    return NULL;
};


void* undo_operations(void* arg)
{


    for (int i = 0; i < NUM_OPERATIONS; i++) {

        sw_tuple_t * pub = NULL;
        sw_tuple_t * prv = NULL;
        pub = dequeue_ts(&buffer);
        //sw_log("%s:%d got tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(pub));
        sw_nat_src_undo(&prv,pub);
        if(prv)
        {
            tuple_checker_dst(prv);
            //sw_tuple_free(prv);
        }
        else
        {
            sw_log("%s:%d not unnating for tuple:%s", __FILE__, __LINE__, sw_tuple_pretty(prv));
        }
        if(prv)
        sw_tuple_free(prv);
        if(pub)
        sw_tuple_free(pub);
    }
        return NULL;
};


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


int main(int argc, char **argv) {
    pthread_t threads[NUM_THREADS+1];


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

    if (sw_debug_flags & SW_DEBUG_CORE)
      sw_log("%s:%d acct backends created", __FILE__, __LINE__);





    //int n = (unsigned int)pthread_self();

    //registerPerr();

    // Create threads for insertions, deletions, and retrievals
    for (int i = 0; i < NUM_THREADS; i++) {

        if (i % 2 == 0) {
            pthread_create(&threads[i], NULL, do_operations, NULL);
        } else {
           pthread_create(&threads[i], NULL, undo_operations, NULL);
        }
    }


    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }



    sw_session_destroy();
    sw_gauge_destroy();
    //sw_shape_destroy();
    //sw_shape_queue_destroy();

    sw_acct_commit(1);
    sw_acct_method_destroy();

    // Clean up and free the hash table


    return 0;
}
