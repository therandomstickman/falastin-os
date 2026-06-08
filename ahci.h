#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>

void ahci_init(void);
int ahci_read_sector(uint32_t lba, uint8_t* buffer);
int ahci_write_sector(uint32_t lba, const uint8_t* buffer);
int ahci_drive_present(void);

#endif