#include "net.h"
#include "rtl8139.h"
#include "screen.h"
#include "libc.h"
#include "timer.h"
#include "fs.h"
#include "lwip/def.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "dns_simple.h"

// ARP cache
ArpCacheEntry arp_cache[16];
int arp_cache_count = 0;

// Our network configuration
static uint8_t our_mac[6];
static uint32_t our_ip = 0;
static uint32_t gateway_ip = 0;
static uint32_t subnet_mask = 0;
static uint32_t dns_server = 0;
static int dhcp_configured = 0;

// HTTP response buffer
static char http_response[8192];
static int http_response_len = 0;
static int http_response_ready = 0;
static struct tcp_pcb* http_pcb = 0;
static int http_connecting = 0;

uint16_t net_checksum(uint16_t* data, uint32_t len) {
    uint32_t sum = 0;
    uint32_t i;
    for (i = 0; i < len / 2; i++) {
        sum += data[i];
    }
    if (len & 1) {
        sum += (data[i] & 0xFF);
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}

void ip_addr_to_str(uint32_t ip, char* buf) {
    snprintf(buf, 16, "%d.%d.%d.%d",
             (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
             (ip >> 8) & 0xFF, ip & 0xFF);
}

uint32_t ip_str_to_addr(const char* ip_str) {
    uint32_t ip = 0;
    uint32_t octet = 0;
    int shift = 24;
    
    for (int i = 0; ip_str[i]; i++) {
        if (ip_str[i] >= '0' && ip_str[i] <= '9') {
            octet = octet * 10 + (ip_str[i] - '0');
        } else if (ip_str[i] == '.') {
            ip |= (octet << shift);
            shift -= 8;
            octet = 0;
        }
    }
    ip |= octet;
    return ip;
}

int is_local_network(uint32_t ip) {
    return (ip & subnet_mask) == (our_ip & subnet_mask);
}

void net_set_static_ip(const char* ip, const char* gateway, const char* mask) {
    our_ip = ip_str_to_addr(ip);
    gateway_ip = ip_str_to_addr(gateway);
    subnet_mask = ip_str_to_addr(mask);
    dhcp_configured = 1;
    
    arp_cache[0].ip = our_ip;
    memcpy(arp_cache[0].mac, our_mac, 6);
    arp_cache[0].valid = 1;
    arp_cache_count = 1;
    
    char buf[16];
    ip_addr_to_str(our_ip, buf);
    print("Static IP set to: ");
    print(buf);
    print("\n");
    ip_addr_to_str(gateway_ip, buf);
    print("Gateway: ");
    print(buf);
    print("\n");
}

void arp_add_static(uint32_t ip, const uint8_t* mac) {
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, 6);
            return;
        }
    }
    if (arp_cache_count < 16) {
        arp_cache[arp_cache_count].ip = ip;
        memcpy(arp_cache[arp_cache_count].mac, mac, 6);
        arp_cache[arp_cache_count].valid = 1;
        arp_cache_count++;
    }
}

void net_init(void) {
    rtl8139_init();
    rtl8139_get_mac(our_mac);
    
    for (int i = 0; i < 16; i++) {
        arp_cache[i].valid = 0;
        arp_cache[i].ip = 0;
    }
    arp_cache_count = 0;
    
    our_ip = ip_str_to_addr("10.0.2.15");
    gateway_ip = ip_str_to_addr("10.0.2.2");
    subnet_mask = ip_str_to_addr("255.255.255.0");
    dns_server = ip_str_to_addr("8.8.8.8");
    
    arp_cache[0].ip = our_ip;
    memcpy(arp_cache[0].mac, our_mac, 6);
    arp_cache[0].valid = 1;
    arp_cache_count = 1;
    
    uint8_t gateway_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    arp_cache[arp_cache_count].ip = gateway_ip;
    memcpy(arp_cache[arp_cache_count].mac, gateway_mac, 6);
    arp_cache[arp_cache_count].valid = 1;
    arp_cache_count++;
    
    char ip_buf[16];
    ip_addr_to_str(our_ip, ip_buf);
    print("IP: ");
    print(ip_buf);
    print("\n");
    ip_addr_to_str(gateway_ip, ip_buf);
    print("Gateway: ");
    print(ip_buf);
    print("\n");
    
    print("MAC: ");
    for (int i = 0; i < 6; i++) {
        print_int(our_mac[i]);
        if (i < 5) print(":");
    }
    print("\n");
    
    print("Network stack ready.\n");
}

void net_send_packet(const uint8_t* data, uint32_t len) {
    rtl8139_send_packet(data, len);
}

void arp_send_request(uint32_t target_ip) {
    uint8_t packet[64];
    EthHeader* eth = (EthHeader*)packet;
    ArpHeader* arp = (ArpHeader*)(packet + sizeof(EthHeader));
    
    memset(eth->dest_mac, 0xFF, 6);
    memcpy(eth->src_mac, our_mac, 6);
    eth->ethertype = 0x0806;
    
    arp->htype = 1;
    arp->ptype = 0x0800;
    arp->hlen = 6;
    arp->plen = 4;
    arp->opcode = 1;
    memcpy(arp->sender_mac, our_mac, 6);
    arp->sender_ip = our_ip;
    memset(arp->target_mac, 0, 6);
    arp->target_ip = target_ip;
    
    rtl8139_send_packet(packet, sizeof(EthHeader) + sizeof(ArpHeader));
}

void arp_handle_packet(uint8_t* packet, uint32_t len) {
    if (len < sizeof(EthHeader) + sizeof(ArpHeader)) return;
    
    ArpHeader* arp = (ArpHeader*)(packet + sizeof(EthHeader));
    uint32_t sender_ip = arp->sender_ip;
    uint32_t target_ip = arp->target_ip;
    uint16_t opcode = arp->opcode;
    
    if (sender_ip == gateway_ip) return;
    
    int found = 0;
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == sender_ip) {
            memcpy(arp_cache[i].mac, arp->sender_mac, 6);
            found = 1;
            break;
        }
    }
    
    if (!found && arp_cache_count < 16) {
        arp_cache[arp_cache_count].ip = sender_ip;
        memcpy(arp_cache[arp_cache_count].mac, arp->sender_mac, 6);
        arp_cache[arp_cache_count].valid = 1;
        arp_cache_count++;
    }
    
    if (opcode == 1 && target_ip == our_ip) {
        uint8_t reply_packet[64];
        EthHeader* reply_eth = (EthHeader*)reply_packet;
        ArpHeader* reply_arp = (ArpHeader*)(reply_packet + sizeof(EthHeader));
        
        memcpy(reply_eth->dest_mac, arp->sender_mac, 6);
        memcpy(reply_eth->src_mac, our_mac, 6);
        reply_eth->ethertype = 0x0806;
        
        reply_arp->htype = 1;
        reply_arp->ptype = 0x0800;
        reply_arp->hlen = 6;
        reply_arp->plen = 4;
        reply_arp->opcode = 2;
        memcpy(reply_arp->sender_mac, our_mac, 6);
        reply_arp->sender_ip = our_ip;
        memcpy(reply_arp->target_mac, arp->sender_mac, 6);
        reply_arp->target_ip = sender_ip;
        
        rtl8139_send_packet(reply_packet, sizeof(EthHeader) + sizeof(ArpHeader));
    }
}

uint8_t* arp_lookup(uint32_t ip) {
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            return arp_cache[i].mac;
        }
    }
    return 0;
}

void icmp_send_reply(uint32_t dest_ip, uint8_t* data, uint32_t len) {
    uint8_t packet[512];
    EthHeader* eth = (EthHeader*)packet;
    IpHeader* ip = (IpHeader*)(packet + sizeof(EthHeader));
    IcmpHeader* icmp = (IcmpHeader*)(packet + sizeof(EthHeader) + sizeof(IpHeader));
    
    uint8_t* dest_mac = arp_lookup(dest_ip);
    if (!dest_mac) {
        arp_send_request(dest_ip);
        return;
    }
    
    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->src_mac, our_mac, 6);
    eth->ethertype = 0x0800;
    
    uint32_t total_len = sizeof(IpHeader) + len;
    ip->ver_ihl = 0x45;
    ip->tos = 0;
    ip->total_len = total_len;
    ip->id = 0x1234;
    ip->flags_off = 0;
    ip->ttl = 64;
    ip->protocol = 1;
    ip->src_ip = our_ip;
    ip->dest_ip = dest_ip;
    ip->checksum = 0;
    ip->checksum = net_checksum((uint16_t*)ip, sizeof(IpHeader));
    
    icmp->type = 0;
    icmp->code = 0;
    icmp->checksum = 0;
    if (data && len > 0) {
        memcpy((uint8_t*)icmp + sizeof(IcmpHeader), data, len);
    }
    icmp->checksum = net_checksum((uint16_t*)icmp, len + sizeof(IcmpHeader));
    
    rtl8139_send_packet(packet, sizeof(EthHeader) + total_len);
}

void icmp_handle_packet(uint8_t* packet, uint32_t len) {
    if (len < sizeof(EthHeader) + sizeof(IpHeader) + sizeof(IcmpHeader)) return;
    
    IpHeader* ip = (IpHeader*)(packet + sizeof(EthHeader));
    IcmpHeader* icmp = (IcmpHeader*)(packet + sizeof(EthHeader) + sizeof(IpHeader));
    
    if (ip->protocol == 1 && icmp->type == 8) {
        uint32_t src_ip = ip->src_ip;
        char ip_buf[16];
        ip_addr_to_str(src_ip, ip_buf);
        print("Ping from ");
        print(ip_buf);
        print("\n");
        
        uint32_t icmp_len = ip->total_len - sizeof(IpHeader);
        uint8_t* icmp_data = (uint8_t*)icmp + sizeof(IcmpHeader);
        uint32_t data_len = icmp_len - sizeof(IcmpHeader);
        
        icmp_send_reply(src_ip, icmp_data, data_len);
    }
}

void ping(uint32_t ip) {
    char ip_buf[16];
    ip_addr_to_str(ip, ip_buf);
    print("Pinging ");
    print(ip_buf);
    print("...\n");
    
    uint32_t target_for_mac = ip;
    if (!is_local_network(ip)) {
        target_for_mac = gateway_ip;
    }
    
    uint8_t* dest_mac = arp_lookup(target_for_mac);
    if (!dest_mac) {
        arp_send_request(target_for_mac);
        timer_sleep(200);
        dest_mac = arp_lookup(target_for_mac);
        if (!dest_mac) return;
    }
    
    uint8_t packet[512];
    EthHeader* eth = (EthHeader*)packet;
    IpHeader* ip_hdr = (IpHeader*)(packet + sizeof(EthHeader));
    IcmpHeader* icmp_hdr = (IcmpHeader*)(packet + sizeof(EthHeader) + sizeof(IpHeader));
    
    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->src_mac, our_mac, 6);
    eth->ethertype = 0x0800;
    
    uint32_t total_len = sizeof(IpHeader) + sizeof(IcmpHeader) + 32;
    ip_hdr->ver_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_len = total_len;
    ip_hdr->id = 0x1234;
    ip_hdr->flags_off = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = 1;
    ip_hdr->src_ip = our_ip;
    ip_hdr->dest_ip = ip;
    ip_hdr->checksum = 0;
    ip_hdr->checksum = net_checksum((uint16_t*)ip_hdr, sizeof(IpHeader));
    
    uint8_t ping_data[32];
    for (int i = 0; i < 32; i++) ping_data[i] = 0x41 + (i % 26);
    
    icmp_hdr->type = 8;
    icmp_hdr->code = 0;
    icmp_hdr->checksum = 0;
    memcpy((uint8_t*)icmp_hdr + sizeof(IcmpHeader), ping_data, 32);
    icmp_hdr->checksum = net_checksum((uint16_t*)icmp_hdr, sizeof(IcmpHeader) + 32);
    
    rtl8139_send_packet(packet, sizeof(EthHeader) + total_len);
}

void net_handle_packet(void) {
    if (!rtl8139_packet_available()) return;
    
    uint8_t packet[2048];
    int len = rtl8139_receive_packet(packet, sizeof(packet));
    if (len <= 0) return;
    
    EthHeader* eth = (EthHeader*)packet;
    
    if (eth->ethertype == 0x0806) {
        arp_handle_packet(packet, len);
    } else if (eth->ethertype == 0x0800) {
        ip_handle_packet(packet, len);
    }
}

void ip_handle_packet(uint8_t* packet, uint32_t len) {
    if (len < sizeof(EthHeader) + sizeof(IpHeader)) return;
    
    IpHeader* ip = (IpHeader*)(packet + sizeof(EthHeader));
    uint32_t dest_ip = ip->dest_ip;
    
    if (dest_ip != our_ip) return;
    
    if (ip->protocol == 1) {
        icmp_handle_packet(packet, len);
    }
}

void net_test_loopback(void) {
    print("Testing network stack (internal loopback)...\n");
    
    uint8_t* dest_mac = arp_lookup(our_ip);
    if (!dest_mac) return;
    
    uint8_t packet[256];
    EthHeader* eth = (EthHeader*)packet;
    IpHeader* ip = (IpHeader*)(packet + sizeof(EthHeader));
    IcmpHeader* icmp = (IcmpHeader*)(packet + sizeof(EthHeader) + sizeof(IpHeader));
    
    memcpy(eth->dest_mac, our_mac, 6);
    memcpy(eth->src_mac, our_mac, 6);
    eth->ethertype = 0x0800;
    
    ip->ver_ihl = 0x45;
    ip->tos = 0;
    uint32_t total_len = sizeof(IpHeader) + sizeof(IcmpHeader) + 32;
    ip->total_len = total_len;
    ip->id = 0x1234;
    ip->flags_off = 0;
    ip->ttl = 64;
    ip->protocol = 1;
    ip->src_ip = our_ip;
    ip->dest_ip = our_ip;
    ip->checksum = 0;
    ip->checksum = net_checksum((uint16_t*)ip, sizeof(IpHeader));
    
    icmp->type = 8;
    icmp->code = 0;
    icmp->checksum = 0;
    uint8_t ping_data[32];
    for (int i = 0; i < 32; i++) ping_data[i] = 0x41 + (i % 26);
    memcpy((uint8_t*)icmp + sizeof(IcmpHeader), ping_data, 32);
    icmp->checksum = net_checksum((uint16_t*)icmp, sizeof(IcmpHeader) + 32);
    
    ip_handle_packet(packet, sizeof(EthHeader) + total_len);
    print("Loopback test complete.\n");
}

// HTTP functions
static err_t http_connected(void* arg, struct tcp_pcb* pcb, err_t err) {
    (void)arg;
    http_connecting = 0;
    
    if (err != ERR_OK || pcb == NULL) {
        http_pcb = 0;
        return ERR_OK;
    }
    
    const char* request = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);
    return ERR_OK;
}

static err_t http_recv(void* arg, struct tcp_pcb* pcb, struct pbuf* p, err_t err) {
    (void)arg;
    if (err != ERR_OK || pcb == NULL) {
        if (pcb) tcp_close(pcb);
        http_pcb = 0;
        http_connecting = 0;
        return ERR_OK;
    }
    
    if (p == NULL) {
        tcp_close(pcb);
        http_pcb = 0;
        http_connecting = 0;
        return ERR_OK;
    }
    
    char* data = (char*)p->payload;
    int len = p->len;
    
    if (http_response_len + len < (int)sizeof(http_response) - 1) {
        memcpy(http_response + http_response_len, data, len);
        http_response_len += len;
        http_response[http_response_len] = '\0';
        http_response_ready = 1;
    }
    
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void http_dns_found(const char* name, const ip_addr_t* ipaddr, void* callback_arg) {
    (void)name;
    (void)callback_arg;
    
    if (ipaddr == NULL) {
        http_connecting = 0;
        return;
    }
    
    http_pcb = tcp_new();
    if (http_pcb == NULL) {
        http_connecting = 0;
        return;
    }
    
    tcp_recv(http_pcb, http_recv);
    tcp_connect(http_pcb, ipaddr, 80, http_connected);
}



void http_get(const char* host, const char* path) {
    (void)path;
    
    http_response_len = 0;
    http_response_ready = 0;
    http_response[0] = '\0';
    
    print("DNS resolving ");
    print(host);
    print("...\n");
    
    uint32_t ip = dns_resolve_simple(host);
    if (ip == 0) {
        print("DNS failed!\n");
        return;
    }
    
    char ip_buf[16];
    ip_addr_to_str(ip, ip_buf);
    print("IP: ");
    print(ip_buf);
    print("\n");
    
    // Continue with TCP connection using the resolved IP
    // For now just store the IP
}

void display_http_response(void) {
    if (!http_response_ready) {
        print("No HTTP response yet. Try again.\n");
        return;
    }
    
    print("\n=== HTTP Response ===\n");
    print(http_response);
    print("\n=== End of Response ===\n");
    
    http_response_ready = 0;
    http_response_len = 0;
}

void browse(const char* url) {
    char host[256];
    char path[256];
    
    char url_copy[512];
    strcpy(url_copy, url);
    
    char* start = url_copy;
    if (strncmp(start, "http://", 7) == 0) {
        start += 7;
    }
    
    char* path_start = strchr(start, '/');
    if (path_start) {
        strncpy(path, path_start, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
        *path_start = '\0';
    } else {
        strcpy(path, "/");
    }
    
    strncpy(host, start, sizeof(host) - 1);
    host[sizeof(host) - 1] = '\0';
    
    http_get(host, path);
}