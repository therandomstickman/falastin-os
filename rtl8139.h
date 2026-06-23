#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

void rtl8139_init(void);
int  rtl8139_send_packet(const uint8_t* data, uint32_t len);
int  rtl8139_receive_packet(uint8_t* buffer, uint32_t max_len);
int  rtl8139_packet_available(void);
void rtl8139_get_mac(uint8_t* mac);

#endif