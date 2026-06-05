#include "storage.h"
#include "screen.h"
#include "ata.h"

#define BLOCK_SIZE 512  // ATA sector size
#define STORAGE_BLOCKS 10240  // 10MB of storage (10240 * 512 = ~5MB)

static int storage_ready = 0;

void storage_init(void)
{
    print("Initializing storage with ATA...\n");
    ata_init();
    
    if (ata_drive_present()) {
        storage_ready = 1;
        print("Storage ready (using real ATA disk)\n");
        print("Files will survive reboots!\n");
    } else {
        print("No ATA drive - storage unavailable\n");
        storage_ready = 0;
    }
}

int storage_read(uint32_t block, uint8_t* buffer)
{
    if (!storage_ready || block >= STORAGE_BLOCKS) {
        return -1;
    }
    
    // Filesystem starts at LBA 100 to avoid boot sector
    uint32_t lba = 100 + block;
    return ata_read_sector(lba, buffer);
}

int storage_write(uint32_t block, const uint8_t* buffer)
{
    if (!storage_ready || block >= STORAGE_BLOCKS) {
        return -1;
    }
    
    // Filesystem starts at LBA 100 to avoid boot sector
    uint32_t lba = 100 + block;
    return ata_write_sector(lba, buffer);
}

int storage_is_present(void)
{
    return storage_ready;
}