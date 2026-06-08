#include "ata.h"
#include "screen.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6

static int ata_present = 0;

static void print_int(int num)
{
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

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void)
{
    inb(0x80);
    inb(0x80);
    inb(0x80);
    inb(0x80);
}

// Wait for drive ready
static int ata_poll(void)
{
    for (int i = 0; i < 1000000; i++) {
        uint8_t status = inb(ATA_PRIMARY_IO + 7);
        if (!(status & 0x80)) {  // BSY cleared
            if (status & 0x01) return -1;  // Error
            return 0;
        }
        io_wait();
    }
    return -1;
}

// Wait for data request
static int ata_poll_drq(void)
{
    for (int i = 0; i < 1000000; i++) {
        uint8_t status = inb(ATA_PRIMARY_IO + 7);
        if (!(status & 0x80)) {
            if (status & 0x08) return 0;  // DRQ set
            if (status & 0x01) return -1;  // Error
        }
        io_wait();
    }
    return -1;
}

static int ata_identify(void)
{
    // Select master
    outb(ATA_PRIMARY_IO + 6, 0xA0);
    io_wait();
    
    // Zero out registers
    outb(ATA_PRIMARY_IO + 2, 0);
    outb(ATA_PRIMARY_IO + 3, 0);
    outb(ATA_PRIMARY_IO + 4, 0);
    outb(ATA_PRIMARY_IO + 5, 0);
    io_wait();
    
    // Send identify command
    outb(ATA_PRIMARY_IO + 7, 0xEC);
    io_wait();
    
    // Check if drive exists
    uint8_t status = inb(ATA_PRIMARY_IO + 7);
    if (status == 0 || status == 0xFF) return -1;
    
    // Poll for ready
    if (ata_poll() != 0) return -1;
    
    // Wait for DRQ
    if (ata_poll_drq() != 0) return -1;
    
    // Read identify data (256 words)
    for (int i = 0; i < 256; i++) {
        inb(ATA_PRIMARY_IO);
        inb(ATA_PRIMARY_IO);
    }
    
    return 0;
}

void ata_init(void)
{
    print("Initializing ATA...\n");
    
    if (ata_identify() == 0) {
        ata_present = 1;
        print("ATA drive detected!\n");
    } else {
        print("No ATA drive found\n");
    }
}

int ata_drive_present(void)
{
    return ata_present;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer)
{
    if (!ata_present) return -1;
    
    // Select drive and LBA
    outb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    
    // Send command
    outb(ATA_PRIMARY_IO + 2, 1);      // 1 sector
    outb(ATA_PRIMARY_IO + 3, lba & 0xFF);
    outb(ATA_PRIMARY_IO + 4, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_IO + 5, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_IO + 7, 0x20);   // Read command
    
    // Wait for data
    if (ata_poll_drq() != 0) return -1;
    
    // Read 256 words
    for (int i = 0; i < 256; i++) {
        uint16_t data = inb(ATA_PRIMARY_IO) | (inb(ATA_PRIMARY_IO) << 8);
        buffer[i*2] = data & 0xFF;
        buffer[i*2+1] = (data >> 8) & 0xFF;
    }
    
    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer)
{
    if (!ata_present) return -1;
    
    // Select drive and LBA
    outb(ATA_PRIMARY_IO + 6, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    
    // Send command
    outb(ATA_PRIMARY_IO + 2, 1);      // 1 sector
    outb(ATA_PRIMARY_IO + 3, lba & 0xFF);
    outb(ATA_PRIMARY_IO + 4, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_IO + 5, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_IO + 7, 0x30);   // Write command
    
    // Wait for DRQ
    if (ata_poll_drq() != 0) return -1;
    
    // Write 256 words
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        outb(ATA_PRIMARY_IO, data & 0xFF);
        outb(ATA_PRIMARY_IO, (data >> 8) & 0xFF);
    }
    
    // Cache flush
    outb(ATA_PRIMARY_IO + 7, 0xE7);
    ata_poll();
    
    return 0;
}