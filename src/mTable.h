#ifndef MTABLE_H
#define MTABLE_H

#include "sweetspot.h"


typedef struct __global_mutex_t_by_peer_t{
    int peer_mtx_pos;
    pthread_mutex_t * mtx_p_table[MAX_MUTEX_BY_PEER];
    pid_t pid_id;
} global_mutex_t_by_peer_t;

extern void mutexesTableInit();
extern void registerPerr();
extern int specMutexLock(pthread_mutex_t * mtx);
extern int specMutexTryLock(pthread_mutex_t * mtx);
extern int specMutexDelLock(pthread_mutex_t * mtx);
extern int releaseAllMtx();
extern int getBlockQty();
extern int releaseLastMtx();
#endif // MTABLE_H


