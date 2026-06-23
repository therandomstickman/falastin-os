#include "rtl8139.h"
#include "pci.h"
#include "screen.h"
#include "libc.h"
#include "timer.h"
#include "fs.h"

#define RTL8139_VENDOR 0x10EC
#define RTL8139_DEVICE 0x8139

// Registers (I/O space)
#define RTL_IDR0     0x00
#define RTL_TSD0     0x10
#define RTL_TSAD0    0x20
#define RTL_RBSTART  0x30
#define RTL_CR       0x37
#define RTL_ISR      0x3E
#define RTL_IMR      0x3C
#define RTL_CAPR     0x38
#define RTL_CBR      0x3A
#define RTL_RCR      0x44

#define CR_RST       0x10
#define CR_RE        0x08
#define CR_TE        0x04

// RCR: accept broadcast, multicast, physical match, and all physical
#define RCR_ACCEPT   0x0F
#define RCR_WRAP     0x80

#define TX_BUF_SIZE  1536

static uint16_t rtl_io_base = 0;
static uint8_t  rtl_mac[6];
static int      rtl_initialized = 0;

#define RX_BUF_SIZE 8192
static uint8_t rx_buffer[RX_BUF_SIZE + 16];
static uint16_t rx_offset = 0;

// Dedicated TX buffer — RTL8139 DMA reads from physical memory
static uint8_t tx_buffer[TX_BUF_SIZE];

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int rtl8139_find_device(void) {
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_read_config(bus, slot, 0, 0);
            uint16_t vendor = vendor_device & 0xFFFF;
            uint16_t device = (vendor_device >> 16) & 0xFFFF;
            if (vendor == 0xFFFF || device == 0xFFFF)
                continue;
            if (vendor == RTL8139_VENDOR && device == RTL8139_DEVICE) {
                uint32_t command = pci_read_config(bus, slot, 0, 0x04);
                command |= (1 << 2) | (1 << 0);  // bus master + I/O space
                pci_write_config(bus, slot, 0, 0x04, command);

                uint32_t bar0 = pci_read_config(bus, slot, 0, 0x10);
                rtl_io_base = bar0 & 0xFFFC;
                print("RTL8139 found at I/O base 0x");
                print_int(rtl_io_base);
                print("\n");
                return 1;
            }
        }
    }
    return 0;
}

void rtl8139_init(void) {
    if (rtl_initialized)
        return;

    print("Initializing RTL8139...\n");

    if (!rtl8139_find_device()) {
        print("RTL8139 not found.\n");
        return;
    }

    // Software reset
    outb(rtl_io_base + RTL_CR, CR_RST);
    for (int i = 0; i < 100000; i++) {
        if (!(inb(rtl_io_base + RTL_CR) & CR_RST))
            break;
    }

    // Read MAC address
    for (int i = 0; i < 6; i++) {
        rtl_mac[i] = inb(rtl_io_base + RTL_IDR0 + i);
    }
    print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        print_int(rtl_mac[i]);
        if (i < 5) print(":");
    }
    print("\n");

    // Receive configuration: accept packets + ring-buffer wrap mode
    outl(rtl_io_base + RTL_RCR, RCR_ACCEPT | RCR_WRAP);

    // Setup receive buffer (physical address; identity-mapped kernel)
    memset(rx_buffer, 0, RX_BUF_SIZE + 16);
    outl(rtl_io_base + RTL_RBSTART, (uint32_t)(uintptr_t)rx_buffer);
    rx_offset = 0;
    outw(rtl_io_base + RTL_CAPR, 0xFFF0);

    // Enable receiver and transmitter
    outb(rtl_io_base + RTL_CR, CR_RE | CR_TE);

    // Clear pending interrupts
    outw(rtl_io_base + RTL_ISR, 0xFFFF);

    rtl_initialized = 1;
    print("RTL8139 initialized.\n");
}

int rtl8139_send_packet(const uint8_t* data, uint32_t len) {
    if (!rtl_initialized || len == 0)
        return -1;
    if (len > TX_BUF_SIZE)
        len = TX_BUF_SIZE;

    // Copy into dedicated DMA buffer
    memcpy(tx_buffer, data, len);

    // Wait for previous transmit to finish (bit 31 clear = done)
    for (int i = 0; i < 100000; i++) {
        if (!(inl(rtl_io_base + RTL_TSD0) & 0x80000000))
            break;
    }

    outl(rtl_io_base + RTL_TSAD0, (uint32_t)(uintptr_t)tx_buffer);
    outl(rtl_io_base + RTL_TSD0, len | 0x80000000);

    return 0;
}

int rtl8139_receive_packet(uint8_t* buffer, uint32_t max_len) {
    if (!rtl_initialized || max_len == 0)
        return -1;

    uint16_t cbr = inw(rtl_io_base + RTL_CBR);
    if (cbr == rx_offset)
        return 0;

    uint16_t* header = (uint16_t*)(rx_buffer + rx_offset);
    uint16_t status = header[0];
    uint16_t packet_len = header[1];

    // Length field includes the 4-byte ring header
    if (packet_len < 4)
        return 0;

    uint16_t frame_len = packet_len - 4;
    if (frame_len > max_len)
        frame_len = (uint16_t)max_len;

    uint8_t* packet_data = rx_buffer + rx_offset + 4;
    memcpy(buffer, packet_data, frame_len);

    // Advance read offset (4-byte aligned)
    rx_offset = (rx_offset + packet_len + 4 + 3) & ~3;
    if (rx_offset >= RX_BUF_SIZE)
        rx_offset = 0;

    // Tell hardware we consumed up to here
    outw(rtl_io_base + RTL_CAPR, rx_offset - 0x10);

    (void)status;
    return (int)frame_len;
}

int rtl8139_packet_available(void) {
    if (!rtl_initialized)
        return 0;
    uint16_t cbr = inw(rtl_io_base + RTL_CBR);
    return cbr != rx_offset;
}

void rtl8139_get_mac(uint8_t* mac) {
    memcpy(mac, rtl_mac, 6);
}

void rtl8139_debug(void) {
    if (!rtl_initialized) {
        print("RTL8139 not initialized\n");
        return;
    }
    print("RTL8139 Debug:\n");
    print("IO Base: 0x");
    print_int(rtl_io_base);
    print("\n");
    print("CR: 0x");
    print_int(inb(rtl_io_base + RTL_CR));
    print("\n");
    print("ISR: 0x");
    print_int(inw(rtl_io_base + RTL_ISR));
    print("\n");
    print("CAPR: 0x");
    print_int(inw(rtl_io_base + RTL_CAPR));
    print("\n");
    print("CBR: 0x");
    print_int(inw(rtl_io_base + RTL_CBR));
    print("\n");
    print("RX offset: 0x");
    print_int(rx_offset);
    print("\n");
}
