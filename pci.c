#include "pci.h"
#include "screen.h"

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}

void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    outl(0xCFC, value);
}

void pci_scan(void)
{
    print("Scanning PCI bus...\n");
    
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_read_config(bus, slot, 0, 0);
            uint16_t vendor_id = vendor_device & 0xFFFF;
            uint16_t device_id = (vendor_device >> 16) & 0xFFFF;
            
            if (vendor_id != 0xFFFF && vendor_id != 0x0000) {
                print("Found device: vendor=0x");
                // Print hex would go here
                print(" slot=");
                print_int(slot);
                print("\n");
            }
        }
    }
}

uint32_t pci_find_device(uint16_t vendor_id, uint16_t device_id)
{
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t vendor_device = pci_read_config(bus, slot, 0, 0);
            if ((vendor_device & 0xFFFF) == vendor_id && 
                ((vendor_device >> 16) & 0xFFFF) == device_id) {
                return (bus << 16) | (slot << 8);
            }
        }
    }
    return 0;
}