#include "ata.h"
#include "screen.h"
#include "diskfs.h"


static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// ADD THESE RIGHT HERE, before any function that uses them
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;  // was just "return" with no value
}

static int ata_present = 0;

static void print_int(int num) {
    char buffer[32];
    int i = 0;
    if (num == 0) {
        put_char('0');
        return;
    }
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        put_char(buffer[j]);
    }
}




static void io_wait(void) {
    inb(0x80);
    inb(0x80);
    inb(0x80);
    inb(0x80);
}

static int ata_poll_bsy(void) {
    int timeout = 1000000;
    while (timeout--) {
        if (!(inb(0x1F7) & 0x80)) return 0;
        io_wait();
    }
    return -1;
}

static int ata_poll_drq(void) {
    int timeout = 1000000;
    while (timeout--) {
        uint8_t status = inb(0x1F7);
        if (status & 0x01) return -1; // error
        if (status & 0x08) return 0;  // DRQ set
        io_wait();
    }
    return -1;
}

static int ata_identify(void) {
    outb(0x1F6, 0xA0);
    io_wait();
    outb(0x1F2, 0);
    outb(0x1F3, 0);
    outb(0x1F4, 0);
    outb(0x1F5, 0);
    outb(0x1F7, 0xEC);
    io_wait();

    uint8_t status = inb(0x1F7);
    if (status == 0 || status == 0xFF) return -1;
    if (ata_poll_bsy() != 0) return -1;

    status = inb(0x1F7);
    if (status & 0x01) return -1;
    if (ata_poll_drq() != 0) return -1;

    // Consume identify data using inw
    for (int i = 0; i < 256; i++)
        inw(0x1F0);

    return 0;
}

void ata_init(void) {
    print("Initializing ATA...\n");
    
    if (ata_identify() == 0) {
        ata_present = 1;
        print("ATA drive detected and initialized.\n");
    } else {
        print("No ATA drive found.\n");
    }
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!ata_present) return -1;

    if (ata_poll_bsy() != 0) return -1;

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);

    if (ata_poll_drq() != 0) return -1;

    // Read 256 WORDS (512 bytes) using inw
    uint16_t* buf16 = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++)
        buf16[i] = inw(0x1F0);

    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (!ata_present) return -1;

    if (ata_poll_bsy() != 0) return -1;

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);

    if (ata_poll_drq() != 0) return -1;

    // Write 256 WORDS using outw
    uint16_t* buf16 = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++)
        outw(0x1F0, buf16[i]);

    // Cache flush
    outb(0x1F7, 0xE7);
    ata_poll_bsy();

    return 0;
}

int ata_drive_present(void) {
    return ata_present;
}