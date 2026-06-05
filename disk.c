#include "disk.h"
#include "screen.h"

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

static void ata_wait(void)
{
    // Wait for drive to be ready
    for (int i = 0; i < 1000; i++) {
        if (!(inb(0x1F7) & 0x80))
            break;
    }
}

void disk_init(void)
{
    print("Initializing ATA drive...\n");
    
    // Detect if drive exists
    outb(0x1F6, 0xA0);  // Select master drive
    
    ata_wait();
    
    uint8_t status = inb(0x1F7);
    if (status == 0xFF) {
        print("No drive detected! Using RAM-only mode.\n");
        return;
    }
    
    print("ATA drive detected\n");
}

int disk_read_sector(uint32_t lba, uint8_t* buffer)
{
    // Wait for drive
    ata_wait();
    
    // Send command
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));  // Master, LBA mode
    outb(0x1F2, 1);      // Read 1 sector
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);   // Read command
    
    // Wait for data
    ata_wait();
    
    // Check for error
    uint8_t status = inb(0x1F7);
    if (status & 0x21) {
        return -1;
    }
    
    // Read 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
        uint16_t data = inb(0x1F0) | (inb(0x1F0) << 8);
        buffer[i*2] = data & 0xFF;
        buffer[i*2+1] = (data >> 8) & 0xFF;
    }
    
    return 0;
}

int disk_write_sector(uint32_t lba, const uint8_t* buffer)
{
    // Wait for drive
    ata_wait();
    
    // Send command
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);      // Write 1 sector
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);   // Write command
    
    // Wait for drive ready
    ata_wait();
    
    // Write 256 words
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        outb(0x1F0, data & 0xFF);
        outb(0x1F0, (data >> 8) & 0xFF);
    }
    
    // Flush cache
    outb(0x1F7, 0xE7);
    ata_wait();
    
    return 0;
}