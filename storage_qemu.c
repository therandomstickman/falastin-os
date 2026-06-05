#include "storage_interface.h"
#include "screen.h"

// This backend uses QEMU's save/load feature to persist RAM storage
// It adds save/load commands but uses RAM storage underneath

static int qemu_init(void)
{
    print("Initializing QEMU persistent storage backend...\n");
    print("Note: Use 'save' to persist, 'load' to restore\n");
    print("Storage will survive reboots within same QEMU session\n");
    return 0;
}

static int qemu_read_sector(uint32_t lba, uint8_t* buffer)
{
    // Forward to RAM backend (we'll use RAM backend under the hood)
    extern StorageBackend ram_backend;
    return ram_backend.read_sector(lba, buffer);
}

static int qemu_write_sector(uint32_t lba, const uint8_t* buffer)
{
    extern StorageBackend ram_backend;
    return ram_backend.write_sector(lba, buffer);
}

static int qemu_present(void)
{
    return 1;
}

static StorageBackend qemu_backend = {
    .name = "qemu",
    .init = qemu_init,
    .read_sector = qemu_read_sector,
    .write_sector = qemu_write_sector,
    .present = qemu_present
};

void qemu_backend_register(void)
{
    storage_register_backend(&qemu_backend);
}

// Helper functions for QEMU persistence
void storage_save_to_qemu(void)
{
    print("To save storage state, use QEMU monitor:\n");
    print("  (Ctrl+Alt+2) then type: savevm storage_state\n");
    print("  (Ctrl+Alt+1) to return\n");
}

void storage_load_from_qemu(void)
{
    print("To load storage state, use QEMU monitor:\n");
    print("  (Ctrl+Alt+2) then type: loadvm storage_state\n");
    print("  (Ctrl+Alt+1) to return\n");
}