#ifndef DISK_H
#define DISK_H

#include <stdint.h>

void disk_init(void);
int disk_read_sector(uint32_t lba, uint8_t* buffer);
int disk_write_sector(uint32_t lba, const uint8_t* buffer);

#endif