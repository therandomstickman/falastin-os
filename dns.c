#include "dns.h"
#include "rtl8139.h"
#include "screen.h"
#include "libc.h"
#include "timer.h"

// DNS header
typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) DnsHeader;

// Build a DNS query
static void build_dns_query(const char* name, uint8_t* buffer, int* len) {
    DnsHeader* header = (DnsHeader*)buffer;
    uint8_t* ptr = buffer + sizeof(DnsHeader);
    
    header->id = 0x1234;
    header->flags = 0x0100;  // Standard query
    header->qdcount = 1;
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    
    // Encode domain name
    const char* pos = name;
    while (*pos) {
        const char* dot = pos;
        while (*dot && *dot != '.') dot++;
        int len = dot - pos;
        *ptr++ = len;
        for (int i = 0; i < len; i++) {
            *ptr++ = pos[i];
        }
        pos = dot;
        if (*pos) pos++;
    }
    *ptr++ = 0;
    
    // QTYPE: A, QCLASS: IN
    *((uint16_t*)ptr) = 0x0001;
    ptr += 2;
    *((uint16_t*)ptr) = 0x0001;
    ptr += 2;
    
    *len = ptr - buffer;
}

// Parse DNS response
static uint32_t parse_dns_response(uint8_t* buffer, int len) {
    if (len < sizeof(DnsHeader)) return 0;
    
    DnsHeader* header = (DnsHeader*)buffer;
    if (header->ancount == 0) return 0;
    
    uint8_t* ptr = buffer + sizeof(DnsHeader);
    
    // Skip query
    while (ptr < buffer + len) {
        if (*ptr == 0) {
            ptr++;
            break;
        }
        ptr += *ptr + 1;
    }
    ptr += 4;  // Skip QTYPE and QCLASS
    
    // Parse answer
    if (header->ancount >= 1) {
        // Skip name (compressed)
        ptr += 2;
        
        uint16_t type = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        uint16_t class_ = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        ptr += 4;  // TTL
        uint16_t data_len = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        
        if (type == 0x0001 && data_len == 4) {
            uint32_t ip = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
            return ip;
        }
    }
    
    return 0;
}

// Send DNS query and wait for response
uint32_t dns_resolve(const char* hostname) {
    uint8_t buffer[512];
    int len;
    
    build_dns_query(hostname, buffer, &len);
    
    // UDP header
    uint8_t packet[1024];
    int pos = 0;
    
    // Ethernet header
    uint8_t gateway_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    memcpy(packet + pos, gateway_mac, 6);
    pos += 6;
    
    // Source MAC (your OS)
    uint8_t our_mac[6];
    rtl8139_get_mac(our_mac);
    memcpy(packet + pos, our_mac, 6);
    pos += 6;
    
    // Ethertype (IP)
    packet[pos++] = 0x08;
    packet[pos++] = 0x00;
    
    // IP header (simplified)
    // Version/IHL, TOS, Length
    packet[pos++] = 0x45;
    packet[pos++] = 0x00;
    uint16_t total_len = 20 + 8 + len;  // IP + UDP + DNS
    packet[pos++] = total_len >> 8;
    packet[pos++] = total_len & 0xFF;
    packet[pos++] = 0x12;
    packet[pos++] = 0x34;
    packet[pos++] = 0x40;
    packet[pos++] = 0x00;
    packet[pos++] = 64;  // TTL
    packet[pos++] = 17;  // UDP
    // Checksum (0 for now)
    packet[pos++] = 0x00;
    packet[pos++] = 0x00;
    // Source IP (10.0.2.15)
    packet[pos++] = 10;
    packet[pos++] = 0;
    packet[pos++] = 2;
    packet[pos++] = 15;
    // Dest IP (DNS server)
    packet[pos++] = 8;
    packet[pos++] = 8;
    packet[pos++] = 8;
    packet[pos++] = 8;
    
    // UDP header
    packet[pos++] = 0x12;
    packet[pos++] = 0x34;  // Source port
    packet[pos++] = 0x00;
    packet[pos++] = 0x35;  // DNS port (53)
    uint16_t udp_len = 8 + len;
    packet[pos++] = udp_len >> 8;
    packet[pos++] = udp_len & 0xFF;
    packet[pos++] = 0x00;
    packet[pos++] = 0x00;  // Checksum (0)
    
    // DNS data
    memcpy(packet + pos, buffer, len);
    pos += len;
    
    // Send packet
    rtl8139_send_packet(packet, pos);
    
    // Wait for response
    for (int i = 0; i < 50; i++) {
        uint8_t recv_buffer[1024];
        if (rtl8139_packet_available()) {
            int recv_len = rtl8139_receive_packet(recv_buffer, sizeof(recv_buffer));
            if (recv_len > 0) {
                // Skip Ethernet and IP headers to get UDP data
                // Check if it's a DNS response
                if (recv_len > 42) {
                    uint32_t ip = parse_dns_response(recv_buffer + 42, recv_len - 42);
                    if (ip != 0) {
                        return ip;
                    }
                }
            }
        }
        timer_sleep(20);
    }
    
    return 0;
}