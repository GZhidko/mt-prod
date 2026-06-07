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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

//#include <netpacket/packet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <net/if_arp.h>


#include "if.h"
#include "cfg.h"
#include "pretty.h"
#include "log.h"
#include <sweetspot.h>
#include "lhstat.h"
#include "netset.h"
//#include "session.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif







// Function to get MAC address from an IP address
int getMACFromIP(const char* ip_address, const char *  ifname , char* mac_address) {
    struct arpreq arpreq;
    struct sockaddr_in* sin;

    memset(&arpreq, 0, sizeof(struct arpreq));
    sin = (struct sockaddr_in*)&arpreq.arp_pa;
    sin->sin_family = AF_INET;
    strncpy(arpreq.arp_dev, ifname, IFNAMSIZ - 1);


    if (inet_pton(AF_INET, ip_address, &sin->sin_addr) <= 0) {
        sw_log("%s:%d inet_pton() failed: %s", __FILE__, __LINE__, strerror(errno));
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        sw_log("%s:%d socket() failed: %s", __FILE__, __LINE__, strerror(errno));
        return -1;
    }

    if (ioctl(sock, SIOCGARP, &arpreq) < 0) {
        sw_log("%s:%d ioctl() failed: %s", __FILE__, __LINE__, strerror(errno));
        close(sock);

        return 0;
    }

    close(sock);
    memcpy(mac_address,&arpreq.arp_ha.sa_data,6);

    // Convert the MAC address to a readable format
    /*snprintf(mac_address, 18, "%hhx:%hhx:%hhx:%02x:%02x:%02x",
             (unsigned char)arpreq.arp_ha.sa_data[0],
             (unsigned char)arpreq.arp_ha.sa_data[1],
             (unsigned char)arpreq.arp_ha.sa_data[2],
             (unsigned char)arpreq.arp_ha.sa_data[3],
             (unsigned char)arpreq.arp_ha.sa_data[4],
             (unsigned char)arpreq.arp_ha.sa_data[5]);*/

    return 0;
}

int sw_if_open_for_input(CONST char *ifname, int fanout_group_id, int * if_idx) {
  int fd;
  struct ifreq ifr;
  struct sockaddr_ll sa;

  memset(&ifr, 0, sizeof(ifr));

  /* Create socket */
  if ((fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL))) < 0) {
    sw_log("%s:%d socket() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  /* Get ifindex */
  strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
  if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
    sw_log("%s:%d ioctl() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  *if_idx = ifr.ifr_ifindex;

  if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
                 (void *)&ifr, sizeof (ifr)) < 0 ) {
    sw_log("%s:%d SO_BINDTODEVICE failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  // Set the SO_RCVBUF option (adjust buffer size as needed)
  int rcvbuf_size = 65536*60; // For example, set the receive buffer size to 8192 bytes
  if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, sizeof(rcvbuf_size)) == -1) {
         sw_log("%s:%d setsockopt() for SO_RCVBUF failed: %s", __FILE__, __LINE__, strerror(errno));
         return -1;
  }

  int sndbuf_size = 65536*60; // For example, set the receive buffer size to 8192 bytes
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size)) == -1) {
         sw_log("%s:%d setsockopt() for SO_RCVBUF failed: %s", __FILE__, __LINE__, strerror(errno));
         return -1;
  }


  if (sw_debug_flags & SW_DEBUG_IO)
    sw_log("%s:%d iface %s, idx %d", __FILE__, __LINE__,
           ifname, ifr.ifr_ifindex);

  /* Bind to particular interface */
  memset(&sa, 0, sizeof(sa));
  sa.sll_family = AF_PACKET;
  sa.sll_protocol = htons(ETH_P_ALL);
  sa.sll_ifindex = ifr.ifr_ifindex;
  sa.sll_pkttype = PACKET_HOST;
  if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    sw_log("%s:%d bind() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (fanout_group_id) {
      // PACKET_FANOUT_LB - round robin
      // PACKET_FANOUT_CPU - send packets to CPU where packet arrived
      int fanout_type = PACKET_FANOUT_HASH;

      int fanout_arg = (fanout_group_id | (fanout_type << 16));

      int setsockopt_fanout = setsockopt(fd, SOL_PACKET, PACKET_FANOUT, &fanout_arg, sizeof(fanout_arg));

      if (setsockopt_fanout < 0) {
          printf("Can't configure fanout\n");
          return -1;
      }
  }

  if (sw_debug_flags & SW_DEBUG_IO)
    sw_log("%s:%d iface %s opened for input as fd %d",
           __FILE__, __LINE__, ifname, fd);

  return fd;
}


int sw_if_close(int sock) {
  close(sock);
  if (sw_debug_flags & SW_DEBUG_IO)
    sw_log("%s:%d socket fd #%d closed", __FILE__, __LINE__, sock);
  return 0;
}


int sw_if_open_for_output(CONST char *ifname) {
  int fd;
  int option=1;
  struct ifreq ifr;

  /* Create socket */
  if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
    sw_log("%s:%d socket() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  /* Make use of original IP header */
  if (setsockopt(fd, IPPROTO_IP, IP_HDRINCL,
                 (CONST void *) &option, sizeof(option)) < 0) {
    sw_log("%s:%d IP_HDRINCL failed: %s", __FILE__, __LINE__,
           strerror(errno));
    return -1;
  }


  /* Bind to particular interface */
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name)-1);
  if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,
                 (void *)&ifr, sizeof (ifr)) < 0 ) {
    sw_log("%s:%d SO_BINDTODEVICE failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  int sndbuf_size = 65536*30; // For example, set the receive buffer size to 8192 bytes
  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf_size, sizeof(sndbuf_size)) == -1) {
         sw_log("%s:%d setsockopt() for SO_RCVBUF failed: %s", __FILE__, __LINE__, strerror(errno));
         return -1;
  }


  if (sw_debug_flags & SW_DEBUG_IO)
    sw_log("%s:%d iface %s opened for output as fd %d",
           __FILE__, __LINE__, ifname, fd);

  return fd;
}




#ifdef AF_PACK_SEND_IMP



int sw_if_send(int fd, char *packet, int length, char isinner, int ifindex) {
  struct sockaddr_ll sa;
  memset(&sa, 0, sizeof(struct sockaddr_ll));
  struct iphdr *iph = (struct iphdr *)packet;

  if (ntohs(iph->tot_len) > length) {
    sw_log("%s:%d short packet %d < %d", __FILE__, __LINE__,
           length, ntohs(iph->tot_len));
    return -1;
  }
  length = ntohs(iph->tot_len);

  sa.sll_family = AF_INET;
  sa.sll_protocol = htons(ETH_P_IP);
  sa.sll_halen = ETH_ALEN;
  sa.sll_ifindex = ifindex;
  if(isinner)
  memcpy(sa.sll_addr, outer_gw_mac, ETH_ALEN);
  else
  memcpy(sa.sll_addr, inner_gw_mac, ETH_ALEN);

  if (sendto(fd, packet, length, 0,
             (struct sockaddr *)&sa, sizeof(struct sockaddr_ll)) != length) {
    if (errno == EACCES) { /* Broadcast address */
      if (sw_debug_flags & SW_DEBUG_IO)
        sw_log("%s:%d sendto %s (broadcast?) failed but ignored",
               __FILE__, __LINE__, sw_pretty_ip(ntohl(iph->daddr)));
      return 0;
    }
    sw_log("%s:%d sendto() failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }

  if (sw_debug_flags & SW_DEBUG_IO)
    sw_log("%s:%d %d-octet packet sent", __FILE__, __LINE__, length);

  return 0;
}
#else


int sw_if_send(int fd, char *packet, int length, char isinner, int ifindex) {
    struct sockaddr_in __dst;
    struct iphdr *iph = (struct iphdr *)packet;
    //change_dst_ip(packet);

    if (ntohs(iph->tot_len) > length) {
      sw_log("%s:%d short packet %d < %d", __FILE__, __LINE__,
             length, ntohs(iph->tot_len));
      return -1;
    }
    length = ntohs(iph->tot_len);

    memset(&__dst, 0, sizeof(__dst));
    __dst.sin_family = AF_INET;
    uint32_t addd = iph->daddr;
//    uint32_t * ip_loc = (uint32_t*)lh_retrieve(global_local_ip,(void*)&addd,NULL);
//    if(ip_loc)
//    {
//        //sw_log("%s:%d find to io: %s", __FILE__, __LINE__, sw_pretty_ip(ntohl(*ip_loc)));
//    if(*ip_loc != iph->daddr)
//    __dst.sin_addr.s_addr = iph->daddr;
//    }
//    else
//     __dst.sin_addr.s_addr = iph->daddr;
    if(!sw_check_ip(local_ip_ns, htonl(addd)))
       __dst.sin_addr.s_addr = iph->daddr;

    if (sendto(fd, packet, length, 0,
               (struct sockaddr *)&__dst, sizeof(__dst)) != length) {
      if (errno == EACCES) { /* Broadcast address */
        if (sw_debug_flags & SW_DEBUG_IO)
          sw_log("%s:%d sendto %s (broadcast?) failed but ignored",
                 __FILE__, __LINE__, sw_pretty_ip(ntohl(iph->daddr)));
        return 0;
      }
      sw_log("%s:%d sendto() failed: %s", __FILE__, __LINE__, strerror(errno));
      return -1;
    }

    if (sw_debug_flags & SW_DEBUG_IO)
      sw_log("%s:%d %d-octet packet sent", __FILE__, __LINE__, length);

    return 0;

}
#endif


int sw_if_recv(int fd, char *packet, int length) {
  struct sockaddr_ll __src;
  static socklen_t __l;
  static int ignore_outgoing_packets = -1;
  int rc;

  if (ignore_outgoing_packets == -1) {
    ignore_outgoing_packets = atoi(sw_config_get("ignore-outgoing-packets", "0"));
  }

  __l = sizeof(__src);
  rc = recvfrom(fd, packet, length, 0, (struct sockaddr *)&__src, &__l);
  if (rc < 0) {
    sw_log("%s:%d recvfrom() failed: %s",
           __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  if (ignore_outgoing_packets && __src.sll_pkttype == PACKET_OUTGOING) {
    if (sw_debug_flags & SW_DEBUG_IO)
      sw_log("%s:%d %d-octet looped back packet ignored",
             __FILE__, __LINE__, rc);
    return 0;
  }

  if (sw_debug_flags & SW_DEBUG_IO)
    sw_log("%s:%d %d-octet packet recved", __FILE__, __LINE__, rc);

  return rc;
}
