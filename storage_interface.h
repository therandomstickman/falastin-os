#ifndef STORAGE_INTERFACE_H
#define STORAGE_INTERFACE_H

#include <stdint.h>

// Storage backend interface
typedef struct {
    const char* name;
    int (*init)(void);
    int (*read_sector)(uint32_t lba, uint8_t* buffer);
    int (*write_sector)(uint32_t lba, const uint8_t* buffer);
    int (*present)(void);
} StorageBackend;

// Register and select backend
void storage_register_backend(StorageBackend* backend);
void storage_select_backend(const char* name);
int storage_init(void);
int storage_read_sector(uint32_t lba, uint8_t* buffer);
int storage_write_sector(uint32_t lba, const uint8_t* buffer);
int storage_present(void);

// List available backends
void storage_list_backends(void);

#endif