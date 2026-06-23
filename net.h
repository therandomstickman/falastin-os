#ifndef NET_H
#define NET_H

#include <stdint.h>

// Ethernet header
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) EthHeader;

// ARP header
typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t opcode;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) ArpHeader;

// IP header
typedef struct {
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) IpHeader;

// ICMP header
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t rest;
} __attribute__((packed)) IcmpHeader;

// ARP cache entry
typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    int valid;
} ArpCacheEntry;

// Network functions
void net_init(void);
void net_send_packet(const uint8_t* data, uint32_t len);
void net_handle_packet(void);

// ARP functions
void arp_send_request(uint32_t target_ip);
void arp_handle_packet(uint8_t* packet, uint32_t len);
uint8_t* arp_lookup(uint32_t ip);

// IP functions
void ip_handle_packet(uint8_t* packet, uint32_t len);

// ICMP functions
void icmp_handle_packet(uint8_t* packet, uint32_t len);
void ping(uint32_t ip);

void lwip_init_port(void);
void lwip_poll(void);
void lwip_ping_test(void);

// HTTP client functions
void browse(const char* url);
void http_get(const char* host, const char* path);
void http_set_callback(void (*callback)(const char* data, int len));
// Add to net.h
void display_http_response(void);
void display_http_response_if_ready(void);

// Helper functions
uint16_t net_checksum(uint16_t* data, uint32_t len);
uint32_t ip_str_to_addr(const char* ip_str);
void ip_addr_to_str(uint32_t ip, char* buf);

// ARP cache (exported)
extern ArpCacheEntry arp_cache[16];
extern int arp_cache_count;

#endif