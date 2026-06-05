#include "ata.h"
#include "pci.h"
#include "screen.h"

#define IDE_PCI_VENDOR_INTEL 0x8086
#define IDE_PCI_DEVICE_PIIX3 0x7010
#define IDE_PCI_VENDOR_VIA 0x1106
#define IDE_PCI_DEVICE_VIA 0x0571

static int ide_initialized = 0;

static void ide_pci_init(void)
{
    // Find IDE controller
    uint32_t ide_dev = pci_find_device(IDE_PCI_VENDOR_INTEL, IDE_PCI_DEVICE_PIIX3);
    if (ide_dev == 0) {
        ide_dev = pci_find_device(IDE_PCI_VENDOR_VIA, IDE_PCI_DEVICE_VIA);
    }
    
    if (ide_dev != 0) {
        print("Found IDE controller\n");
        
        uint8_t bus = (ide_dev >> 16) & 0xFF;
        uint8_t slot = (ide_dev >> 8) & 0xFF;
        
        // Read BARs (Base Address Registers)
        uint32_t bar0 = pci_read_config(bus, slot, 0, 0x10) & 0xFFFFFFFC;
        uint32_t bar1 = pci_read_config(bus, slot, 0, 0x14) & 0xFFFFFFFC;
        uint32_t bar2 = pci_read_config(bus, slot, 0, 0x18) & 0xFFFFFFFC;
        uint32_t bar3 = pci_read_config(bus, slot, 0, 0x1C) & 0xFFFFFFFC;
        
        print("IDE BARs: ");
        print_int(bar0);
        print(" ");
        print_int(bar1);
        print(" ");
        print_int(bar2);
        print(" ");
        print_int(bar3);
        print("\n");
        
        // Enable bus mastering
        uint32_t command = pci_read_config(bus, slot, 0, 0x04);
        command |= (1 << 2); // Bus master enable
        pci_write_config(bus, slot, 0, 0x04, command);
    }
}

void ata_init(void)
{
    print("Initializing IDE/SATA controller...\n");
    
    pci_scan();
    ide_pci_init();
    
    // Now use standard ATA PIO but with PCI timing
    // Try to detect drives
    for (int i = 0; i < 4; i++) {
        uint16_t io_base = (i < 2) ? 0x1F0 : 0x170;
        uint8_t drive = (i % 2) ? 0xB0 : 0xA0;
        
        // Select drive
        outb(io_base + 6, drive);
        
        // Try to read status
        uint8_t status = inb(io_base + 7);
        if (status != 0xFF) {
            print("Drive detected on channel ");
            print_int(i);
            print("\n");
            ide_initialized = 1;
        }
    }
    
    if (!ide_initialized) {
        print("No IDE/SATA drives found\n");
    } else {
        print("IDE/SATA initialized\n");
    }
}

int ata_drive_present(void)
{
    return ide_initialized;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer)
{
    if (!ide_initialized) return -1;
    
    // Use primary channel for simplicity
    uint16_t io_base = 0x1F0;
    
    // Wait for ready
    for (int timeout = 0; timeout < 1000000; timeout++) {
        uint8_t status = inb(io_base + 7);
        if (!(status & 0x80)) break;
    }
    
    // Select drive and LBA mode
    outb(io_base + 6, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Send command
    outb(io_base + 2, 1);  // 1 sector
    outb(io_base + 3, (uint8_t)lba);
    outb(io_base + 4, (uint8_t)(lba >> 8));
    outb(io_base + 5, (uint8_t)(lba >> 16));
    outb(io_base + 7, 0x20);  // Read command
    
    // Wait for DRQ
    for (int timeout = 0; timeout < 1000000; timeout++) {
        uint8_t status = inb(io_base + 7);
        if (status & 0x08) break;
        if (status & 0x01) return -1;
    }
    
    // Read data
    for (int i = 0; i < 256; i++) {
        uint16_t data = inb(io_base) | (inb(io_base) << 8);
        buffer[i*2] = data & 0xFF;
        buffer[i*2+1] = (data >> 8) & 0xFF;
    }
    
    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t* buffer)
{
    if (!ide_initialized) return -1;
    
    uint16_t io_base = 0x1F0;
    
    // Wait for ready
    for (int timeout = 0; timeout < 1000000; timeout++) {
        uint8_t status = inb(io_base + 7);
        if (!(status & 0x80)) break;
    }
    
    // Select drive and LBA mode
    outb(io_base + 6, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Send command
    outb(io_base + 2, 1);
    outb(io_base + 3, (uint8_t)lba);
    outb(io_base + 4, (uint8_t)(lba >> 8));
    outb(io_base + 5, (uint8_t)(lba >> 16));
    outb(io_base + 7, 0x30);  // Write command
    
    // Wait for DRQ
    for (int timeout = 0; timeout < 1000000; timeout++) {
        uint8_t status = inb(io_base + 7);
        if (status & 0x08) break;
        if (status & 0x01) return -1;
    }
    
    // Write data
    for (int i = 0; i < 256; i++) {
        uint16_t data = buffer[i*2] | (buffer[i*2+1] << 8);
        outb(io_base, data & 0xFF);
        outb(io_base, (data >> 8) & 0xFF);
    }
    
    // Cache flush
    outb(io_base + 7, 0xE7);
    
    return 0;
}