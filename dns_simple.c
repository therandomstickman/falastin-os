#include "dns_simple.h"
#include "rtl8139.h"
#include "screen.h"
#include "libc.h"
#include "timer.h"

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) DnsHeader;

static uint8_t our_mac[6];
static uint8_t gateway_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

static void build_dns_query(const char* name, uint8_t* buffer, int* len) {
    DnsHeader* header = (DnsHeader*)buffer;
    uint8_t* ptr = buffer + sizeof(DnsHeader);
    
    header->id = 0x1234;
    header->flags = 0x0100;
    header->qdcount = 1;
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    
    while (*name) {
        const char* dot = name;
        while (*dot && *dot != '.') dot++;
        int seg_len = dot - name;
        *ptr++ = seg_len;
        for (int i = 0; i < seg_len; i++) {
            *ptr++ = name[i];
        }
        name = dot;
        if (*name) name++;
    }
    *ptr++ = 0;
    
    *((uint16_t*)ptr) = 0x0001;
    ptr += 2;
    *((uint16_t*)ptr) = 0x0001;
    ptr += 2;
    
    *len = ptr - buffer;
}

static uint32_t parse_dns_response(uint8_t* buffer, int len) {
    if (len < sizeof(DnsHeader)) return 0;
    DnsHeader* header = (DnsHeader*)buffer;
    if (header->ancount == 0) return 0;
    
    uint8_t* ptr = buffer + sizeof(DnsHeader);
    while (ptr < buffer + len) {
        if (*ptr == 0) { ptr++; break; }
        ptr += *ptr + 1;
    }
    ptr += 4;
    
    if (header->ancount >= 1) {
        ptr += 2;
        uint16_t type = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        ptr += 2;
        ptr += 4;
        uint16_t data_len = (ptr[0] << 8) | ptr[1];
        ptr += 2;
        if (type == 0x0001 && data_len == 4) {
            return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        }
    }
    return 0;
}

uint32_t dns_resolve_simple(const char* hostname) {
    rtl8139_get_mac(our_mac);
    
    uint8_t buffer[512];
    int len;
    build_dns_query(hostname, buffer, &len);
    
    uint8_t packet[1024];
    int pos = 0;
    
    // Ethernet header
    memcpy(packet + pos, gateway_mac, 6);
    pos += 6;
    memcpy(packet + pos, our_mac, 6);
    pos += 6;
    packet[pos++] = 0x08;
    packet[pos++] = 0x00;
    
    // IP header
    packet[pos++] = 0x45;
    packet[pos++] = 0x00;
    uint16_t total_len = 20 + 8 + len;
    packet[pos++] = total_len >> 8;
    packet[pos++] = total_len & 0xFF;
    packet[pos++] = 0x12;
    packet[pos++] = 0x34;
    packet[pos++] = 0x40;
    packet[pos++] = 0x00;
    packet[pos++] = 64;
    packet[pos++] = 17;
    packet[pos++] = 0x00;
    packet[pos++] = 0x00;
    packet[pos++] = 10;
    packet[pos++] = 0;
    packet[pos++] = 2;
    packet[pos++] = 15;
    packet[pos++] = 8;
    packet[pos++] = 8;
    packet[pos++] = 8;
    packet[pos++] = 8;
    
    // UDP header
    packet[pos++] = 0x12;
    packet[pos++] = 0x34;
    packet[pos++] = 0x00;
    packet[pos++] = 0x35;
    uint16_t udp_len = 8 + len;
    packet[pos++] = udp_len >> 8;
    packet[pos++] = udp_len & 0xFF;
    packet[pos++] = 0x00;
    packet[pos++] = 0x00;
    
    // DNS data
    memcpy(packet + pos, buffer, len);
    pos += len;
    
    rtl8139_send_packet(packet, pos);
    
    for (int i = 0; i < 30; i++) {
        if (rtl8139_packet_available()) {
            uint8_t recv_buffer[1024];
            int recv_len = rtl8139_receive_packet(recv_buffer, sizeof(recv_buffer));
            if (recv_len > 42) {
                uint32_t ip = parse_dns_response(recv_buffer + 42, recv_len - 42);
                if (ip != 0) {
                    return ip;
                }
            }
        }
        timer_sleep(20);
    }
    return 0;
}