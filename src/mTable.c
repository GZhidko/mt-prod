#include "mTable.h"
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <sys/types.h>
#include "log.h"
#include "assert.h"
#include "syscall.h"
#include "unistd.h"
#include "string.h"
#include "errno.h"

global_mutex_t_by_peer_t g_peerMutex_table[MAX_PROC_RCV_CNT+4];


void mutexesTableInit()
{
    memset(&g_peerMutex_table,0x0,sizeof(g_peerMutex_table));
};

void registerPerr()
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == 0)
        {
           g_peerMutex_table[i].pid_id = p_id;
           return;
        }
    }
    sw_log("%s:%d pid mutex table is full:%d", __FILE__, __LINE__,
          p_id);

}

int specMutexLock(pthread_mutex_t * mtx)
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == p_id)
        {
           pthread_mutex_lock(mtx);
           g_peerMutex_table[i].mtx_p_table[g_peerMutex_table[i].peer_mtx_pos] = mtx;
           g_peerMutex_table[i].peer_mtx_pos++;
           assert(g_peerMutex_table[i].peer_mtx_pos < MAX_MUTEX_BY_PEER );
           return 1;
        }
    }
    sw_log("%s:%d tid not found or not registered in mtx table tid:%d", __FILE__, __LINE__,
          p_id);
    return -1;
};

int specMutexTryLock(pthread_mutex_t * mtx)
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == p_id)
        {
           if(mtx->__data.__owner == p_id) return 0;
           int ret = pthread_mutex_trylock(mtx);
           if(ret==0)
           {
            g_peerMutex_table[i].mtx_p_table[g_peerMutex_table[i].peer_mtx_pos] = mtx;
            g_peerMutex_table[i].peer_mtx_pos++;
            assert(g_peerMutex_table[i].peer_mtx_pos < MAX_MUTEX_BY_PEER );
            return 0;
           }
           else
               return -1;
        }
    }
    sw_log("%s:%d tid not found or not registered in mtx table tid:%d", __FILE__, __LINE__,
          p_id);
    return -1;
};


int specMutexDelLock(pthread_mutex_t * mtx)
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == p_id)
        {
           if(mtx->__data.__owner == p_id)
           {
             for(int p = 0; p < g_peerMutex_table[i].peer_mtx_pos;++p)
             {
                 if(g_peerMutex_table[i].mtx_p_table[p] == mtx)
                 {
                     g_peerMutex_table[i].mtx_p_table[p] =  NULL;
                     break;
                 }
             }
             return 0;
           }
           int ret = pthread_mutex_trylock(mtx);
           if(ret==0)
           {
            return 0;
           }
           else
               return -1;
        }
    }
    sw_log("%s:%d tid not found or not registered in mtx table tid:%d", __FILE__, __LINE__,
          p_id);
    return -1;
};



int releaseAllMtx()
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == p_id)
        {
           if(g_peerMutex_table[i].peer_mtx_pos>0)
//           sw_log("%s:%d releasing %d blocking for tid:%d", __FILE__, __LINE__,
//                   g_peerMutex_table[i].peer_mtx_pos,p_id);
           while(g_peerMutex_table[i].peer_mtx_pos > 0)
           {
               g_peerMutex_table[i].peer_mtx_pos--;
               if(g_peerMutex_table[i].mtx_p_table[g_peerMutex_table[i].peer_mtx_pos])
               pthread_mutex_unlock(g_peerMutex_table[i].mtx_p_table[g_peerMutex_table[i].peer_mtx_pos]);
           }
           return 1;
        }
    }

};


int releaseLastMtx()
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == p_id)
        {
               if(g_peerMutex_table[i].peer_mtx_pos>0)
//           sw_log("%s:%d releasing %d blocking for tid:%d", __FILE__, __LINE__,
//                   g_peerMutex_table[i].peer_mtx_pos,p_id);

               g_peerMutex_table[i].peer_mtx_pos--;
               if(g_peerMutex_table[i].mtx_p_table[g_peerMutex_table[i].peer_mtx_pos])
               pthread_mutex_unlock(g_peerMutex_table[i].mtx_p_table[g_peerMutex_table[i].peer_mtx_pos]);

           return 1;
        }
    }

};

int getBlockQty()
{
    pid_t p_id = syscall(SYS_gettid);
    for(int i = 0; i<MAX_PROC_RCV_CNT+4;++i)
    {
        if(g_peerMutex_table[i].pid_id == p_id)
        {
           return g_peerMutex_table[i].peer_mtx_pos;
        }
    }
    return -1;
}
