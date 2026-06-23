#include "lwip/ip_addr.h"
#include "lwip/inet_chksum.h"
#include "lwip/def.h"
#include "lwip/pbuf.h"

u16_t inet_chksum(const void *dataptr, u16_t len) {
    const u16_t *data = (const u16_t*)dataptr;
    u32_t sum = 0;
    u16_t i;
    
    for (i = 0; i < len / 2; i++) {
        sum += data[i];
    }
    
    if (len & 1) {
        sum += ((u16_t)((const u8_t*)dataptr)[len - 1] << 8);
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

u16_t inet_chksum_pbuf(struct pbuf *p) {
    u32_t sum = 0;
    struct pbuf *q;
    
    for (q = p; q != NULL; q = q->next) {
        sum += inet_chksum(q->payload, q->len);
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

u16_t ip_chksum_pseudo(struct pbuf *p, u8_t proto, u16_t proto_len, const ip_addr_t *src, const ip_addr_t *dest) {
    u32_t sum = 0;
    
    sum += (src->addr >> 16) & 0xFFFF;
    sum += src->addr & 0xFFFF;
    sum += (dest->addr >> 16) & 0xFFFF;
    sum += dest->addr & 0xFFFF;
    sum += htons(proto);
    sum += htons(proto_len);
    
    struct pbuf *q;
    for (q = p; q != NULL; q = q->next) {
        sum += inet_chksum(q->payload, q->len);
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}
