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

#ifndef __SH_QUEUE_H__
#define __SH_QUEUE_H__

#include "sweetspot.h"
#include "uammsg.h"
#include <netinet/in.h>

#define QMAX 16

typedef struct __rcv_pckt{
    sw_socket_handler_t * sw_sh;
    char * buf;
    int length;
    struct sockaddr_in addr;
    char cyphertext[SW_UAM_MSG_SIZE];

}rcv_pckt_t;

typedef struct __sh_queue {
  rcv_pckt_t  qu[QMAX];
  int read_pos, wrt_pos;
}sh_queue_t;

void init(sh_queue_t *q);
int get_wrt_pos(sh_queue_t *q);
rcv_pckt_t * get_data(sh_queue_t *q);
void free_pos(sh_queue_t *q);
int isempty(sh_queue_t *q);
//int isempty(sh_queue_t *q);
//sw_socket_handler_t * pop(sh_queue_t *q);


#endif
