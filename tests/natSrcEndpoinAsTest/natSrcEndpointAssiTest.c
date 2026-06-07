#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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
#include "sweetspot.h"
int global_alive_flag =  1;

sw_socket_handler_t ** global_socket_handlers;
int process_qnty = 0;

#define NUM_THREADS 30
#define NUM_OPERATIONS 5000

sw_tuple_t * tuple_generator(int ip, int port)
{
    sw_tuple_t * tuple = (sw_tuple_t*)malloc(sizeof(sw_tuple_t));
    sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)malloc(sizeof(sw_tuple_ip_t));
    tuple->protocol = IPPROTO_IP;
    sw_tuple_ip->ip = ip;
    tuple->prv =  (sw_tuple_ip_t*)sw_tuple_ip;
    sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)malloc(sizeof(sw_tuple_tcp_t));
    tuple->next = (sw_tuple_t*)malloc(sizeof(sw_tuple_t));
    sw_tuple_tcp->port = port;
    tuple->next->prv = (sw_tuple_tcp_t*) sw_tuple_tcp;
    tuple->next->protocol = IPPROTO_TCP;
    return tuple;
}


int tuple_checker(int ip_low, int ip_high, int port_low, int port_high, sw_tuple_t * tuple)
{
    sw_tuple_t * tuple_ = tuple;
    if(tuple->protocol != IPPROTO_IP || tuple->next->protocol != IPPROTO_TCP)
    {
        printf("wrong protocol tuple:%s", sw_tuple_pretty(tuple_));
        return -1;
    }
    sw_tuple_ip_t * sw_tuple_ip = (sw_tuple_ip_t*)tuple->prv;
    if(sw_tuple_ip->ip < ip_low && sw_tuple_ip->ip >= ip_high)
    {
        printf("wrong IP tuple:%s", sw_tuple_pretty(tuple_));
        return -1;
    }
    sw_tuple_tcp_t * sw_tuple_tcp = (sw_tuple_tcp_t*)tuple->next->prv;
    if(sw_tuple_tcp->port < port_low && sw_tuple_tcp->port >= port_high)
    {
        printf("wrong PORT tuple:%s", sw_tuple_pretty(tuple_));
        return -1;
    }
    return 0;
};


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

void* create_operations(void* arg)
{
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            uint32_t random_number = generate_random_uint32(2130706433, 2130706446);
            if (sw_nat_src_enabled_flag &&
                sw_nat_src_endpoint_new(global_ns, random_number) == -1) {
              break;
            }
        }
        return NULL;
};


void* delete_operations(void* arg)
{
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            uint32_t random_number = generate_random_uint32(2130706433, 2130706446);
            if (sw_nat_src_enabled_flag &&
                sw_nat_src_endpoint_del(global_ns, random_number) == -1) {
              break;
            }
        }
        return NULL;
};



void* assign_operations(void* arg)
{
        for (int i = 0; i < NUM_OPERATIONS; i++) {
            uint32_t random_number = generate_random_uint32(2130706433, 2130706446);
            uint32_t random_port = generate_random_uint32(2130706433, 2130706446);
            sw_tuple_t * pub = tuple_generator(random_number, 80);
            sw_tuple_t * prv = tuple_generator(235929601, 80);
            if (sw_nat_src_endpoint_assign(pub, prv, 0) == -1) {
              sw_tuple_free(pub);
              //pthread_rwlock_unlock(&nat_src_lock_rw);
              return -1;
            }
        }
        return NULL;
};



int main() {
    pthread_t threads[NUM_THREADS];
    sw_log_init("test_endp_assign.log");

;
    //FILE fd = open(global_logfile, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    // Initialize the hash table
    sw_debug_create("all");

    if (sw_config_load("natSrcEndpoinAsTest/test.conf") < 0)
      return -1;

    CONST char *user_cidrs = sw_config_get("user-networks", NULL);
    if (sw_netset_create(&global_ns, user_cidrs) == -1) {
      return -1;
    }

    if (sw_nat_src_endpoint_create(global_ns) < 0) {
      return -1;
    }

    time_t now = time(NULL);

    for (uint32_t i= 2130706433; i< 2130706447; i++)
    {
        //printf("create endpoint for ip:%s", sw_pretty_ip(i));
    if (sw_nat_src_enabled_flag &&
        sw_nat_src_endpoint_new(global_ns, i) == -1) {
      return -1;
    }
    }
    // Create threads for insertions, deletions, and retrievals
    /*for (int i = 0; i < NUM_THREADS; i++) {
        if (i % 3 == 0) {
            pthread_create(&threads[i], NULL, insert_operations, NULL);
        } else if (i % 3 == 1) {
            pthread_create(&threads[i], NULL, delete_operations, NULL);
        } else {
            pthread_create(&threads[i], NULL, retrieve_operations, NULL);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }*/
    now = time(NULL);
    // Clean up and free the hash table


    return 0;
}
