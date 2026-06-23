#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// Bare-metal: no OS threads; main loop drives timers and RX
#define NO_SYS                      1
#define LWIP_NETCONN                0
#define LWIP_SOCKET                 0
#define LWIP_CALLBACK_API           1
#define SYS_LIGHTWEIGHT_PROT        0

// Disable all debug output
#define LWIP_DEBUG 0
#define LWIP_DBG_MIN_LEVEL 0
#define LWIP_DBG_TYPES_ON 0

// Disable TCP sanity checks
#define LWIP_DISABLE_TCP_SANITY_CHECKS 1

// Memory settings
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    32768
#define MEMP_NUM_PBUF               16
#define MEMP_NUM_RAW_PCB            4
#define MEMP_NUM_UDP_PCB            4
#define MEMP_NUM_TCP_PCB            4
#define MEMP_NUM_TCP_PCB_LISTEN     4
#define MEMP_NUM_TCP_SEG            16
#define MEMP_NUM_ARP_QUEUE          4
#define MEMP_NUM_NETIF              1
#define MEMP_NUM_SYS_TIMEOUT        16

// ARP settings
#define ARP_TABLE_SIZE              10
#define ARP_QUEUEING                1
#define ETHARP_TRUST_IP_MAC         1

// IP settings
#define IP_FORWARD                  1
#define IP_OPTIONS_ALLOWED          0
#define IP_REASSEMBLY               1
#define IP_FRAG                     1
#define IP_REASS_MAX_PBUFS          10
#define IP_DEFAULT_TTL              64

// ICMP settings
#define ICMP_TTL                    64
#define LWIP_ICMP                   1

// TCP settings
#define LWIP_TCP                    1
#define TCP_TTL                     64
#define TCP_WND                     4096
#define TCP_SND_BUF                 4096
#define TCP_SND_QUEUELEN            16
#define TCP_MSS                     1460
#define TCP_SNDLOWAT                1024
#define TCP_SNDQUEUELOWAT           512
#define TCP_MAXRTX                  12
#define TCP_SYNMAXRTX               6
#define TCP_QUEUE_OOSEQ             1
#define TCP_OOSEQ_MAX_BYTES         4096
#define TCP_OOSEQ_MAX_PBUFS         4

// UDP settings
#define LWIP_UDP                    1
#define UDP_TTL                     64

// DHCP settings
#define LWIP_DHCP                   1
#define DHCP_DOES_ARP_CHECK         0

// DNS settings
#define LWIP_DNS                    1
#define DNS_TABLE_SIZE              4
#define DNS_MAX_NAME_LENGTH         64
#define DNS_MAX_SERVERS             2
#define DNS_FALLBACK                1

// Statistics - disable
#define LWIP_STATS                  0
#define LWIP_STATS_DISPLAY          0

// Checksum settings
#define LWIP_CHKSUM_ALGORITHM       1
#define CHECKSUM_GEN_IP             1
#define CHECKSUM_GEN_UDP            1
#define CHECKSUM_GEN_TCP            1
#define CHECKSUM_GEN_ICMP           1
#define CHECKSUM_CHECK_IP           1
#define CHECKSUM_CHECK_UDP          1
#define CHECKSUM_CHECK_TCP          1
#define CHECKSUM_CHECK_ICMP         1

// Socket API
#define LWIP_SOCKET                 0
#define LWIP_COMPAT_SOCKETS         0
#define LWIP_POSIX_SOCKETS_IO_NAMES 0

// Netif settings
#define LWIP_NETIF_STATUS_CALLBACK  0
#define LWIP_NETIF_LINK_CALLBACK    0
#define LWIP_NETIF_HOSTNAME         0

// Ethernet
#define ETH_PAD_SIZE                0
#define LWIP_ETHERNET               1

#endif