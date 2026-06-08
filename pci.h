#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_scan(void);
uint32_t pci_find_device(uint16_t vendor_id, uint16_t device_id);

#endif