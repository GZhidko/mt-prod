#ifndef __SW_CACHE_H__
#define __SW_CACHE_H__

#include "stdint.h"
#include "portab.h"
#include "filter.h"



#define MAX_CAPACITY 100
#define MAX_CACHE_COUNT 1000



typedef struct __CachVal__
{
    sw_filter_rule_t * rule;    // Pointer to the rule of the filter
    int type;                   // Type of the cache value
} CachVal;

// Structure for representing a node in the cache
typedef struct __StaticCacheNode__ {
    uint32_t key;               // Key of the cached data
    CachVal value;              // Value associated with the key
    time_t last_dl_time;        // Last download time
    int counter;                // Counter
    struct __StaticCacheNode__* prev;   // Pointer to the previous node
    struct __StaticCacheNode__* next;   // Pointer to the next node
} StaticCacheNode;

// Structure for the cache
typedef struct __StaticCache__ {
    StaticCacheNode* front;     // Pointer to the front of the cache
    StaticCacheNode* rear;      // Pointer to the rear of the cache
    int size;                   // Size of the cache
} StaticCache;



extern void initCache PROTO((StaticCache* cache));        // Initialize the cache
extern void put PROTO((StaticCache* cache, uint32_t key, CachVal value));   // Insert a new element into the cache
extern int get PROTO((StaticCache* cache, uint32_t key, CachVal ** cv));    // Retrieve the value based on the key from the cache
extern void resetCache PROTO((StaticCache* cache));
#endif // CACHE_H
