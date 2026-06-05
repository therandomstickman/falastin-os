#include "storage_interface.h"
#include "screen.h"
#include "ata.h"

static int ata_storage_init(void)
{
    print("Initializing ATA storage backend...\n");
    ata_init();
    
    if (ata_drive_present()) {
        print("ATA storage ready (real disk)\n");
        return 0;
    }
    
    print("ATA storage not available\n");
    return -1;
}

static int ata_storage_read_sector(uint32_t lba, uint8_t* buffer)
{
    // Filesystem starts at LBA 100 to avoid boot sector
    return ata_read_sector(100 + lba, buffer);
}

static int ata_storage_write_sector(uint32_t lba, const uint8_t* buffer)
{
    // Filesystem starts at LBA 100 to avoid boot sector
    return ata_write_sector(100 + lba, buffer);
}

static int ata_storage_present(void)
{
    return ata_drive_present();
}

static StorageBackend ata_backend = {
    .name = "ata",
    .init = ata_storage_init,
    .read_sector = ata_storage_read_sector,
    .write_sector = ata_storage_write_sector,
    .present = ata_storage_present
};

void ata_backend_register(void)
{
    storage_register_backend(&ata_backend);
}