#include <spinlock.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "log.h"
#include <execinfo.h>

// util
#define MAX_SPIN_ATTEMPTS 1000000
//#define UNBLOK_MODE

static inline bool atomic_compare_exchange(int* ptr, int compare, int exchange) {
    return __atomic_compare_exchange_n(ptr, &compare, exchange,
            0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline void atomic_store(int* ptr, int value) {
    __atomic_store_n(ptr, 0, __ATOMIC_SEQ_CST);
}

static inline int atomic_add_fetch(int* ptr, int d) {
    return __atomic_add_fetch(ptr, d, __ATOMIC_SEQ_CST);
}


void spinlock_lock(spinlock* spinlock) {
    if (spinlock != NULL) {
        int attempts = 0;
        //sw_log("%s:%d Warning: Spinlock attempts exceeded limit force unblocking (%d)", __FILE__, __LINE__,MAX_SPIN_ATTEMPTS);
        while (!atomic_compare_exchange(&spinlock->locked, 0, 1)) {
           #ifdef UNBLOK_MODE
            attempts++;
            if (attempts >= MAX_SPIN_ATTEMPTS) {
                sw_log("%s:%d Warning: Spinlock attempts exceeded limit force unblocking (%d)", __FILE__, __LINE__,MAX_SPIN_ATTEMPTS);
                break;
            }
            #endif
        }
    }
}

void spinlock_unlock(spinlock* spinlock)
{   if(spinlock!= NULL)
    atomic_store(&spinlock->locked, 0);
}

