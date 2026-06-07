#ifndef MUTEXCURULARBUFFER_H
#define MUTEXCURULARBUFFER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include "portab.h"
#include "tuple.h"
#include "spinlock.h"

typedef struct
{
    sw_tuple_t* chain;
    uint64_t octets;
    int direction;
} gauge_tuple_t;


// Define the structure of a node in the linked list
typedef struct Node {
    gauge_tuple_t *data;
    struct Node *next;
} Node;

// Define the structure of the circular buffer
typedef struct {
    Node *head;
    Node *tail;
    spinlock lock; // Replace spinlock with your actual spin lock implementation
} CircularBufferGauge;

extern void init_buffer PROTO((CircularBufferGauge *buffer));

extern bool enqueue PROTO((CircularBufferGauge *buffer, gauge_tuple_t * value));

extern gauge_tuple_t* dequeue PROTO((CircularBufferGauge *buffer)) ;

#endif // MUTEXCURULARBUFFER_H
