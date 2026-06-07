#include "sh_queue.h"
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>




void init(sh_queue_t *q) {
  q->wrt_pos = 0;
  q->read_pos = 0;

  for(int i=0;i<QMAX;++i){
  q->qu[i].buf = (char*)malloc(32768*sizeof(char));
    if(q->qu[i].buf == NULL)
      sw_log("%s:%d mem alocation error",
             __FILE__, __LINE__);
  memset(q->qu->buf, 0, sizeof(32768));
  q->qu[i].length = 0;
  }
  return;
}

int get_wrt_pos(sh_queue_t *q){
    int cur_pos = q->wrt_pos ;
    int next_pos = 0;

    if(q->wrt_pos < QMAX-1)
        next_pos = cur_pos +1;
    else
        next_pos = 0;

    if(next_pos == q->read_pos) {
        sw_log(" sh_queue is full pos hnd:%s :%d ", q->qu[q->read_pos].sw_sh->name,q->read_pos);
        return -1;
    }
    q->wrt_pos = next_pos;
    return cur_pos;
}

rcv_pckt_t * get_data(sh_queue_t *q){
    return &q->qu[q->read_pos];
}

void free_pos(sh_queue_t *q)
{
    //memset(q->qu[q->read_pos].cyphertext,0x0,SW_UAM_MSG_SIZE);
    if(q->read_pos < QMAX-1)
        q->read_pos++;
    else
        q->read_pos = 0;
}


int isempty(sh_queue_t *q){

    return (q->wrt_pos == q->read_pos);
}
