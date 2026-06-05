#include "storage_interface.h"
#include "screen.h"

#define MAX_BACKENDS 4

static StorageBackend* backends[MAX_BACKENDS];
static int backend_count = 0;
static StorageBackend* current_backend = NULL;

void storage_register_backend(StorageBackend* backend)
{
    if (backend_count < MAX_BACKENDS) {
        backends[backend_count++] = backend;
        print("Registered storage backend: ");
        print(backend->name);
        print("\n");
    }
}

void storage_select_backend(const char* name)
{
    for (int i = 0; i < backend_count; i++) {
        int match = 1;
        const char* a = backends[i]->name;
        const char* b = name;
        while (*a && *b && *a == *b) {
            a++; b++;
        }
        if (*a == *b) {
            current_backend = backends[i];
            print("Selected storage backend: ");
            print(current_backend->name);
            print("\n");
            return;
        }
    }
    print("Backend not found: ");
    print(name);
    print("\n");
}

int storage_init(void)
{
    if (current_backend && current_backend->init) {
        return current_backend->init();
    }
    return -1;
}

int storage_read_sector(uint32_t lba, uint8_t* buffer)
{
    if (current_backend && current_backend->read_sector) {
        return current_backend->read_sector(lba, buffer);
    }
    return -1;
}

int storage_write_sector(uint32_t lba, const uint8_t* buffer)
{
    if (current_backend && current_backend->write_sector) {
        return current_backend->write_sector(lba, buffer);
    }
    return -1;
}

int storage_present(void)
{
    if (current_backend && current_backend->present) {
        return current_backend->present();
    }
    return 0;
}

void storage_list_backends(void)
{
    print("Available storage backends:\n");
    for (int i = 0; i < backend_count; i++) {
        print("  - ");
        print(backends[i]->name);
        if (current_backend == backends[i]) {
            print(" (active)");
        }
        print("\n");
    }
}