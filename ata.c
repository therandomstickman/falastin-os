#include "ata.h"
#include "screen.h"

#define ATA_PRIMARY_IO 0x1F0
#define ATA_PRIMARY_CONTROL 0x3F6

#define ATA_REG_DATA 0
#define ATA_REG_ERROR 1
#define ATA_REG_SECTORS 2
#define ATA_REG_LBA_LOW 3
#define ATA_REG_LBA_MID 4
#define ATA_REG_LBA_HIGH 5
#define ATA_REG_DRIVE 6
#define ATA_REG_COMMAND 7
#define ATA_REG_STATUS 7

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_FLUSH_CACHE 0xE7

#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERR 0x01

static int ata_present = 0;

static void ata_print_int(int num)
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
    for (int i = 0; i < 10; i++) {
        inb(0x80);
    }
}

static int ata_wait_ready(void)
{
    uint8_t status;
    int timeout = 10000000;
    
    while (timeout > 0) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY)) {
            return 0;
        }
        timeout--;
        io_wait();
    }
    
    print("ATA timeout waiting for ready\n");
    return -1;
}

static int ata_wait_drq(void)
{
    uint8_t status;
    int timeout = 10000000;
    
    while (timeout > 0) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR) {
            print("ATA error: ");
            ata_print_int(inb(ATA_PRIMARY_IO + ATA_REG_ERROR));
            print("\n");
            return -1;
        }
        if (status & ATA_STATUS_DRQ) {
            return 0;
        }
        timeout--;
        io_wait();
    }
    
    print("ATA timeout waiting for DRQ\n");
    return -1;
}

static int ata_identify(uint16_t* buffer, int slave)
{
    uint8_t drive_select = slave ? 0xB0 : 0xA0;
    
    // Select drive
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, drive_select);
    io_wait();
    
    // Zero out registers
    outb(ATA_PRIMARY_IO + ATA_REG_SECTORS, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, 0);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, 0);
    
    // Send identify command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    io_wait();
    
    // Check if drive exists
    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0) {
        return -1;
    }
    
    // Wait for ready
    if (ata_wait_ready() != 0) return -1;
    
    // Check for error
    status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status & ATA_STATUS_ERR) {
        return -1;
    }
    
    // Check if it's an ATAPI device (CD-ROM)
    uint8_t lba_mid = inb(ATA_PRIMARY_IO + ATA_REG_LBA_MID);
    uint8_t lba_high = inb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH);
    
    if (lba_mid != 0 || lba_high != 0) {
        return -1;
    }
    
    // Wait for data
    if (ata_wait_drq() != 0) return -1;
    
    // Read identification data
    for (int i = 0; i < 256; i++) {
        buffer[i] = inb(ATA_PRIMARY_IO + ATA_REG_DATA) | 
                   (inb(ATA_PRIMARY_IO + ATA_REG_DATA) << 8);
    }
    
    return 0;
}

void ata_init(void)
{
    print("Initializing ATA driver...\n");
    
    uint16_t identify_buffer[256];
    
    // Try master
    print("Checking master drive...\n");
    if (ata_identify(identify_buffer, 0) == 0) {
        ata_present = 1;
        print("Hard drive detected as master!\n");
        
        // Print drive model
        print("Model: ");
        for (int i = 27; i <= 46; i++) {
            uint16_t word = identify_buffer[i];
            char c1 = (word >> 8) & 0xFF;
            char c2 = word & 0xFF;
            if (c1 >= 32 && c1 <= 126) put_char(c1);
            if (c2 >= 32 && c2 <= 126) put_char(c2);
        }
        print("\n");
        return;
    }
    
    print("No ATA hard drive found!\n");
}

void ata_test(void)
{
    print("Running ATA test...\n");
    
    uint8_t test_buffer[512];
    
    // Initialize test pattern
    for (int i = 0; i < 512; i++) {
        test_buffer[i] = i % 256;
    }
    
    print("Writing test pattern to LBA 100...\n");
    if (ata_write_sector(100, test_buffer) != 0) {
        print("Write failed!\n");
        return;
    }
    
    print("Reading back from LBA 100...\n");
    uint8_t read_buffer[512];
    if (ata_read_sector(100, read_buffer) != 0) {
        print("Read failed!\n");
        return;
    }
    
    // Compare
    int match = 1;
    for (int i = 0; i < 512; i++) {
        if (test_buffer[i] != read_buffer[i]) {
            print("Mismatch at byte ");
            ata_print_int(i);
            print(": wrote ");
            ata_print_int(test_buffer[i]);
            print(", got ");
            ata_print_int(read_buffer[i]);
            print("\n");
            match = 0;
            break;
        }
    }
    
    if (match) {
        print("ATA test PASSED! Storage works perfectly!\n");
    } else {
        print("ATA test FAILED!\n");
    }
}

int ata_drive_present(void)
{
    return ata_present;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer)
{
    // Wait 400ns between commands (inb(0x80) gives ~1us delay)
    for (int i = 0; i < 4; i++) inb(0x80);
    
    // Select drive
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    for (int i = 0; i < 4; i++) inb(0x80);
    
    // Send command
    outb(0x1F2, 1);  // 1 sector
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);  // Read command
    
    // Wait for DRQ (Data Request)
    uint8_t status;
    int timeout = 100000;
    do {
        status = inb(0x1F7);
        timeout--;
        if (timeout == 0) return -1;
        for (int i = 0; i < 4; i++) inb(0x80);
    } while (!(status & 0x08) && !(status & 0x01));
    
    if (status & 0x01) return -1;  // Error
    
    // Read data
    for (int i = 0; i < 256; i++) {
        uint16_t data = inb(0x1F0) | (inb(0x1F0) << 8);
        buffer[i*2] = data & 0xFF;
        buffer[i*2+1] = (data >> 8) & 0xFF;
    }
    
    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer)
{
    if (!ata_present) return -1;
    
    if (ata_wait_ready() != 0) return -1;
    
    // Select drive and LBA mode
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    io_wait();
    
    // Set parameters
    outb(ATA_PRIMARY_IO + ATA_REG_SECTORS, 1);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    
    // Send write command
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    io_wait();
    
    // Wait for DRQ
    if (ata_wait_drq() != 0) return -1;
    
    // Write data
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        outb(ATA_PRIMARY_IO + ATA_REG_DATA, data & 0xFF);
        outb(ATA_PRIMARY_IO + ATA_REG_DATA, (data >> 8) & 0xFF);
    }
    
    // Flush cache
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
    ata_wait_ready();
    
    return 0;
}