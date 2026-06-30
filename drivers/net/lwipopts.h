#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

#define NO_SYS                      1

#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

#define LWIP_IPV4                   1
#define LWIP_ICMP                   1 
#define LWIP_RAW                    1
#define LWIP_UDP                    1
#define LWIP_TCP                    1
#define LWIP_DHCP                   1 
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_DNS                    1

#define TCP_MSS                     1460   

#define TCP_WND                 65535

#define TCP_SND_BUF                 (22 * TCP_MSS)

#define TCP_SND_QUEUELEN            (4 * (TCP_SND_BUF / TCP_MSS))

#define MEMP_NUM_TCP_SEG            132

#define MEMP_NUM_TCP_PCB            4
#define MEMP_NUM_TCP_PCB_LISTEN     4

#define MEM_LIBC_MALLOC             0
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (128 * 1024)

#define PBUF_POOL_SIZE          64
#define PBUF_POOL_BUFSIZE       1536 

#define MEMP_NUM_PBUF               16

#define LWIP_NOASSERT               1

#endif